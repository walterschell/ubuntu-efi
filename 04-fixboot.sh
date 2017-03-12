#!/bin/bash
echo "*** Make sure to delete the shim boot manager ***"
efibootmgr -v
echo "Paste below"
echo "efibootmgr -B -b XXXX"
