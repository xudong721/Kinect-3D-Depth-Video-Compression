#!/bin/bash
## This program encodes and decodes all the layers that it finds inside the given folder
##
## input:
## 	layers_basename	-- the basename of the layers folders
## 	videos_folder	-- directory where to create the video files
## 	decode_folder	-- folder where to put the images of the layers decoded from the videos
## 	layers_number	-- the number of layers that have been used to represent the images
## 	quality_profile	-- quality profile to be used to encode the layers

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


if [ $# -lt 5 ]
then
	echo "";
	echo "Usage: ${0##*/} <layers_basename> <videos_folder> <decode_folder> <layers_number> <quality_profile>";
	echo "";
	echo "read the script for further details on the inputs";
	exit 0;
fi


lay_basename=$1;
video_fold=$2;
deco_fold=$3;
layers_num=$4;
qp=$5;

# prune tailing slashes from the folders names
while [ ${video_fold:${#video_fold}-1} = '/' ]; do video_fold=${video_fold:0:${#video_fold}-1}; done
while [ ${deco_fold:${#deco_fold}-1} = '/' ]; do deco_fold=${deco_fold:0:${#deco_fold}-1}; done


# create the output folders that don't exist
if [ ! -e $video_fold ]
then
	mkdir -p $video_fold;
fi

if [ ! -e $deco_fold ]
then
	mkdir -p $deco_fold;
fi


dset_name=${lay_basename##*/};

lay_digits=1;
pow=10;
while [ $pow -lt $layers_num ]
do
	(( lay_digits ++ ));
	(( pow = 10 ** $lay_digits ));
done
unset pow;

numformat='%0';
numformat=${numformat}${lay_digits}d;


# encode and the decode the idmap
idname=${dset_name}_idmap;
id_video_name=${video_fold}/${idname}.mp4

# the idmap is always encoded losslessly
./encode_and_decode.sh ${lay_basename}_idmap $id_video_name $deco_fold/${idname} 0;


# encode and decode the layers
count=0;
while [ $count -lt $layers_num ]
do
	num=`printf $numformat $count`;
	name=${dset_name}_${num};

	video_name=$video_fold/${name}.mp4;

	./encode_and_decode.sh ${lay_basename}_${num} $video_name $deco_fold/${name} $qp;

	(( count++ ));
done

# copy the intervals setting file to the decoded folder
cp ${lay_basename}_layers.txt ${deco_fold}/${dset_name}_layers.txt
