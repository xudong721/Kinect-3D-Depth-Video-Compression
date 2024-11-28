#!/bin/bash
##
## This program uses x264 and ffmpeg to encode and decode a set of images using the given quality profile
##
## input:
## 	input_folder		-- the folder containing the png files to be encoded
## 	video_output_file	-- video file to be created when encoding
## 	decode_folder		-- folder where to put the decoded images
## 	quality_profile		-- quality profile to be used when encoding
##				   (0 is lossless, 69 is the worse)

#
# Copyright (C) 2014 Fabrizio Nenci and Luciano Spinello and Cyrill Stachniss
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#


if [ $# -lt 4 ]
then
	echo "Usage: ${0##*/} <input_folder> <video_output_file> <decode_folder> <quality_profile>";
	exit 0;
fi

if [ -e $3 ]
then
	rm -rf $3;
fi

mkdir $3;

exec_folder=`pwd`;

cd $1;
images=`ls *.png`;
cd $exec_folder;

# count the images
numofimages=0;
for i in $images
do
	(( numofimages++ ));

done

digits_needed=1;
pow=10
while [ $pow -lt $numofimages ]
do
	(( digits_needed ++ ));
	(( pow = 10 ** $digits_needed ));
done

# cut tailing '/' from input and output folders
in_fold=$1;
out_fold=$3;
while [ ${in_fold:${#in_fold}-1} = '/' ]; do in_fold=${in_fold:0:${#in_fold}-1}; done
while [ ${out_fold:${#out_fold}-1} = '/' ]; do out_fold=${out_fold:0:${#out_fold}-1}; done

# encode the images into a video file
x264 --output-csp rgb --qp $4 -o $2 ${in_fold}/%0${digits_needed}d.png --fps 10;

# decode the video into images
ffmpeg -i $2 -r 10 -f image2 ${out_fold}/%0${digits_needed}d.png;

