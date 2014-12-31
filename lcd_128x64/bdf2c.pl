#! /usr/bin/perl

$name = $ARGV[0];
$name =~ s/\.bdf$//i;
$name =~ s/-/_/g;

# This configures which characters to extract from the BDF file.
# Normally a complete ASCII font would cover 32 to 126, but it
# may be useful to extract a smaller range if only certain
# characters are needed.

($first, $last) = (32, 126);	# all normal ascii characters
#($first, $last) = (65, 90);	# capital letters only
#($first, $last) = (43, 57);	# numbers and numerical symbols only


# How tall are the characters?  We need to know, since the BDF format
# indexes the height from the base line, but we're going to index from
# the "top" (since it greatly simplifies the 8051-based code).  Normally
# CAP_HEIGHT tells how tall a capital letter is.  I have carefully
# adjusted CAP_HEIGHT in these fonts where lowercase letters like 'b'
# or numbers like '4' extend above, so using CAP_HEIGHT is a good
# compromise which avoids truncating "normal" characters.  However, some
# symbols like $, %, /, | mat get a pixel or two shaved off their top.
# 
$char_height_str = 'CAP_HEIGHT';
#
# An alternate approach would be to use FONT_ASCENT instead of
# CAP_HEIGHT.  This will include the extra space above all
# characters, even though very few (if any) protrude above the
# top of capital letters.  Typically parentheses, square brackets,
# quotes, and some symbols like $, #, %, ~, | are the only one.
# Normally these will get their top pixel or two clipped off.
# If you don't like that, switch to FONT_ASCENT.  Some fonts have
# utterly wrong FONT_ASCENT.
#
# $char_height_str = 'FONT_ASCENT';


# Read the BDF file
while (<>) {
	if (/^$char_height_str ([0-9]+)/) {
		$char_height = $1;
	}
	if (/^FULL_NAME /) {
		$full_name = $';
		$full_name =~ s/\s+$//;
	}
	if (/^STARTCHAR (.+)/) {
		$startchar = $1;
		undef $encoding, $dwidth, $bbx_w, $bbx_h, $bbx_xoff, $bbx_yoff;
	}
	if (/^ENCODING ([0-9]+)/ && $1 >= $first && $1 <= $last) {
		$encoding = $1;
	}
	if (defined($encoding) && /^DWIDTH ([0-9]+)/) {
		$dwidth = $1;
	}
	if (defined($encoding) && /^BBX ([0-9]+) ([0-9]+) ([-0-9]+) ([-0-9]+)/) {
		($bbx_w, $bbx_h, $bbx_xoff, $bbx_yoff) = ($1, $2, $3, $4);
		if ($bbx_w > 32) {
			warn "$name: char $startchar ($encoding) too wide, truncate to 32 pixels\n";
			$bbx_w = 32;
		}
		if ($bbx_h > 32) {
			warn "$name: char $startchar ($encoding) too tall, truncate to 32 pixels\n";
			$bbx_h = 32;
		}
	}
	if (defined($encoding) && defined($bbx_w) && defined($dwidth) && /^BITMAP/) {
		$linecount = 0;
		while (<>) {
			last if /^ENDCHAR/;
			s/\s+$//;
			if (length($_) > 8) {
				$_ = substr($_, 0, 8);
			}
			for ($i=length($_); $i<8; $i++) {
				$_ .= '0';
			}
			$bitmap[$encoding][$linecount++] = hex($_);
		}
		if ($linecount == $bbx_h && $linecount <= 32) {
			$name[$encoding] = $startchar;
			$width[$encoding] = $bbx_w;
			$height[$encoding] = $bbx_h;
			$xoffset[$encoding] = $bbx_xoff;
			$yoffset[$encoding] = $bbx_yoff;
			$advance[$encoding] = $dwidth;
		}
		undef $startchar,$encoding,$dwidth,$bbx_w,$bbx_h,$bbx_xoff,$bbx_yoff;
	}
}

if (!defined($char_height)) {
	warn "$name: $char_height_str not defined!\n";
}

#trim white space off
for ($char = $first; $char <= $last; $char++) {
	next unless defined($height[$char]);
	#print "// trim character $char, height $height[$char]\n";
	while ($height[$char] > 1 && $bitmap[$char][0] == 0) {
		#print "trimming top of character $char\n";
		$height[$char]--;
		for ($i=0; $i < $height[$char]; $i++) {
			$bitmap[$char][$i] = $bitmap[$char][$i + 1];
		}
		#TODO: adjust $yoffset[character]
	}

	while ($height[$char] > 1 && $bitmap[$char][$height[$char]-1] == 0) {
		#print "// trimming bottom of character $char\n";
		$height[$char]--;
		$yoffset[$char]++;	#TODO: is this correct?
	}
	$bits = 0;
	for ($line=0; $line < $height[$char]; $line++) {
		$bits |= $bitmap[$char][$line];
	}
	while ($width[$char] > 1 && ($bits & 0x80000000) == 0) {
		#print "// trimming left of character $char\n";
		$width[$char]--;
		$bits <<= 1;
		for ($line=0; $line < $height[$char]; $line++) {
			$bitmap[$char][$line] <<= 1;
		}
		$xoffset[$char]++;
	}

	while ($width[$char] > 1 && ($bits & (1 << (32 - $width[$char]))) == 0) {
		#printf "// trimming right of character $char, bits = %X,", $bits;
		#printf "//  width = $width[$char], mask = %X\n", (1 << (32 - $width[$char]));
		$width[$char]--;
	}
}

#clear any extra "garbage" left after real data (transpose may read it)
for ($char = $first; $char <= $last; $char++) {
	for ($i=$height[$char]; $i < 32; $i++) {
		$bitmap[$char][$i] = 0;
	}
}

#create all the output bytes
$bytecount = 0;
$out = '';
for ($char = $first; $char <= $last; $char++) {
	# if this character doesn't exit, output a 3 byte non-printing character
	if (!defined($height[$char])) {
		$size[$char] = 3;
		$out .= "// not defined ($char)\n";
		$out .= "0x00,0x08,0x00";
		$out .= ',' if ($char != $last);
		$out .= "\n\n";
		next;
	}

	# compute y offset
	$yoff = $char_height - ($height[$char] + $yoffset[$char]);
	if ($yoff < 0) {
		# todo, shift it down??  is that better than clipping??
		warn "$name: Character $name[$char] ($char) too tall, clipping $yoff pixels\n";
		while ($yoff < 0) {
			for ($i=0; $i<$height[$char]-1; $i++) {
				$bitmap[$char][$i] = $bitmap[$char][$i+1];
			}
			$height[$char]--;
			$yoff++;
		}
	}
	$num_row = int(($height[$char] + 7) / 8);

	# compute leading and trailing space
	$lead = $xoffset[$char];
	$trail = $advance[$char] - ($width[$char] + $lead);
	if ($lead < -4) { $lead = -4; }
	if ($lead > 11) { $lead = 11; }
	if ($trail < -5) { $trail = -5; }
	if ($trail > 10) { $trail = 10; }

	# print info about this character
	$out .= "// $name[$char] ($char): w=$width[$char], h=$height[$char]";
	#$out .= ", rows=$num_row, yoff=$yoff, lead=$lead, trail=$trail";
	$out .= "\n";
	
	# output 2 or 3 byte header
	if ($yoff <= 7 &&  $lead >= -1 && $lead <= 3 && $trail >= -2 && $trail <= 3) {
		$out .= sprintf "0x%02X,0x%02X,\n",
			($width[$char] - 1) | (($num_row - 1) << 5),
			($yoff << 5) | ((($lead + 1) * 6) + ($trail + 2));
		$bytecount += 2;
		$size[$char] = 2;
	} else {
		$out .= sprintf "0x%02X,0x%02X,0x%02X,\n",
			($width[$char] - 1) | (($num_row - 1) << 5) | 128,
			$yoff,
			(($lead + 4) << 4) | ($trail + 5);
		$bytecount += 3;
		$size[$char] = 3;
	}

	# transpose and output the bitmap
	for ($row=0; $row < $num_row; $row++) {
		for ($x=0; $x < $width[$char]; $x++) {
			$byte = 0;
			for ($bit=0; $bit<8; $bit++) {
				if ($bitmap[$char][$row * 8 + $bit] & (1 << (31 - $x))) {
					$byte |= (1 << $bit) 
				}
			}
			$out .= sprintf "0x%02X", $byte;
			$out .= ',' if ($char != $last || $x < $width[$char] - 1 || $row < $num_row - 1);
			$out .= "\n" if $x == 9 && $width[$char] > 10;
			$out .= "\n" if $x == 19 && $width[$char] > 20;
			$bytecount++;
			$size[$char]++;
		}
		$out .= "\n";
	}
	$out .= "\n";
}

#write the .c file
print "#include \"lcd_buffer.h\"\t\t\t// for FAST_FONT_INDEX\n";
print "\n// $name = $full_name\n\n" if defined($full_name);
print "const unsigned char font_$name\[\] = {\n";
printf "0x%02X,0x%02X,   // characters $first to $last\n", $first, $last - $first;
$bytecount += 2;
print "#ifdef FAST_FONT_INDEX\n";
$offset = 2 + ($last - $first + 1) * 2;
for ($char = $first; $char <= $last; $char++) {
	printf "0x%02X,0x%02X,\n", $offset & 255, ($offset >> 8) & 255;
	$offset += $size[$char];
}
print "#endif // FAST_FONT_INDEX\n";
print "\n";
print $out;
print "};\n";
print "// $bytecount bytes\n";
print "// ", $bytecount + ($last - $first + 1) * 2, " bytes with FAST_FONT_INDEX\n";


#add this font definition to the .h file
open H, ">>lcd_fonts.h";
print H "extern code unsigned char font_$name\[\];";
print H "\t// $full_name" if defined($full_name);
print H "\n";
close H;




