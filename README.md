# fit-image-tests
A simple test program containing code for analyzing and working with FIT OpenWRT kernel images.

U-Boots libfdt is prohibitively large and complex, so I wrote my own simplified version of an
fdt_get_prop to extract the Linux kernel from a OpenWRT fit image.
