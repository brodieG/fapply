
dir <- "tmp/new/"
x <- readLines(file.path(dir, 'r2c.asm'))

# Drop the specific location lines since the comments include the c instruction
x <- grep("; /Users", x, value=TRUE, invert=TRUE)

# Find all the addresses and capture them (we know max address is 3 digits).
rexe <- regexec("^ *[a-f0-9]+:[ 0-9a-f]+ .*?0x([a-f0-9]{3})", x, perl=TRUE)
match <- regmatches(x, rexe)
address <- unique(vapply(match[lengths(match) == 2], "[", "", 2))

# Give each address an index
address.i <- sprintf("x%03d", seq_along(address))

# Replace each address with the corresponding index
for(i in seq_along(address))
  x <- gsub(sprintf("( |0x)%s\\b", address[i]), address.i[i], x)

# Remove unneeded address
x <- gsub("^( )+ [a-f0-9]+:", "\\1    :", x)
writeLines(x, file.path(dir, 'r2c-2.asm'))



