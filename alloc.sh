#!/bin/bash

cat example.wlp4 | wlp4scan | wlp4parse | ./a.out | cs241.linkasm > output.merl
cs241.linker output.merl alloc.merl | cs241.merl 0 > final.mips
mips.twoints final.mips