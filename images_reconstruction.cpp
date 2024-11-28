// This program reassembles images starting from their component layers

// Copyright (C) 2014 Fabrizio Nenci and Luciano Spinello and Cyrill Stachniss

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "layers_merger.h"
#include "opencv2/opencv.hpp"
#include <iostream>
#include <fstream>
#include <stdlib.h>     // exit
#include <string.h>	// strcpy
#include <dirent.h>

unsigned int _rows;
unsigned int _cols;

std::vector< std::pair<unsigned short, unsigned short> > _intervals;

char _name_images_folder [1024];
char _name_output_folder [1024];


// simple implementation of the bubble sort algorithm that works on strings
void orderStringsVector(std::vector<std::string> & strings){
	for (unsigned int i=1; i<strings.size(); i++){
		for (unsigned int k=i; k>0; k--){
			if (strings[k] < strings[k-1]){
				// switch the two strings
				std::string tmp = strings[k-1];
				strings[k-1] = strings[k];
				strings[k] = tmp;
			}
			else{
				break;
			}
		}
	}
}



void init(int argc, char ** argv)
{
	if(argc < 3)
	{
		std::cout << std::endl << "Usage: " << argv[0] << " <images_folders_basename> <output_folder>" << std::endl << std::endl;
		exit(0);
	}

	strcpy(_name_images_folder, argv[1]);
	strcpy(_name_output_folder, argv[2]);
	
	std::string _name_layers_file(_name_images_folder);
	_name_layers_file.append("_layers.txt");

	std::ifstream iin(_name_layers_file.c_str());
	
	if(!iin){
		std::cerr << "ERROR: cannot read file " << _name_layers_file << std::endl;
		exit(1);
	}
	
	// read the rows and columns
	iin >> _rows >> _cols;

	// read the layers far and near cuts
	{
		unsigned int n;
		unsigned int f;

		iin >> n;

		while(iin.good())
		{
			iin >> f;

			_intervals.push_back(std::pair<unsigned short, unsigned short>(n,f));

			iin >> n;
		}

	}

	iin.close();

}



int main(int argc, char ** argv)
{
	init(argc, argv);
	
	std::cout << std::endl << "==== IMAGES RECONSTRUCTION ====" << std::endl << std::endl;
	std::cout << "images_basename:\t" << _name_images_folder << std::endl;
	std::cout << "output folder:\t\t" << _name_output_folder << std::endl;
	std::cout << "rows x cols:\t" << _rows << "x" << _cols << std::endl;
	std::cout << "intervals:";
	for(unsigned int i=0; i<_intervals.size(); i++)
	{
		std::cout <<"\t[" << _intervals[i].first << "," << _intervals[i].second << "]";
	}
	std::cout << std::endl;
	
	unsigned int total_cells = _rows * _cols;
	
	// prepare structures for hosting the images
	unsigned char * idmap = static_cast<unsigned char *>(malloc(sizeof(unsigned char) * total_cells));

	std::vector<unsigned char *> layers;
	for(unsigned int i=0; i<_intervals.size(); i++)
	{
		unsigned char * lay = static_cast<unsigned char *>(malloc(sizeof(unsigned char) * total_cells));
		layers.push_back(lay);
	}

	unsigned short * reconstructed = static_cast<unsigned short *>(malloc(sizeof(unsigned short) * total_cells));
	
	unsigned int * r_idx = static_cast<unsigned int *>(malloc(sizeof(unsigned int) * _rows));
	{
		unsigned int count = 0;
		for(unsigned int i=0; i<_rows; i++)
		{
			r_idx[i] = count;
			count += _cols;
		}
	}

	// create the layers merger
	ds::LayersMerger layers_merger(_intervals, _rows, _cols);


	// prepare folders names
	std::string idmap_folder_name(_name_images_folder);
	idmap_folder_name.append("_idmap");

	std::cout << "idmap_folder_name:\t" << idmap_folder_name << std::endl;
	
	std::string format(_name_images_folder);
	{
		format.append("_%0");
		unsigned int digits_needed = 1;
		while(pow(10, digits_needed) < _intervals.size()) digits_needed++;

		char dgtbuff[32];
		int written = sprintf(dgtbuff, "%d", digits_needed);
		dgtbuff[written] = 0;

		format.append(dgtbuff);
		format.append("d");
	}

	std::vector<std::string> layers_folders_names;
	for(unsigned int i=0; i<_intervals.size(); i++)
	{
		char buff[1024];
		sprintf(buff, format.c_str(), i);

		std::string name(buff);

		layers_folders_names.push_back(name);

		std::cout << "layers_folders_names[" << i << "]:\t" << layers_folders_names[i] << std::endl;
	}

	// create the output directory
	{
		DIR* dir = opendir(_name_output_folder);
		if(dir)
		{
			// remove the directory
			char rmcmd[1024];
			sprintf(rmcmd, "rm -rf %s", _name_output_folder);
			system(rmcmd);
			closedir(dir);
		}
		// create the directory
		char mkcmd[1024];
		sprintf(mkcmd, "mkdir -p %s", _name_output_folder);
		system(mkcmd);
	}

	std::vector<std::string> filenames;
	// populate filenames vector
	{
		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir (idmap_folder_name.c_str())) != NULL)
		{
			/* list all the files and directories within directory */
			while ((ent = readdir (dir)) != NULL)
			{
				std::string name(ent->d_name);
				if(name.compare(".") == 0  ||  name.compare("..") == 0) continue;
				filenames.push_back(name);
			}
			closedir (dir);
		}
		else
		{
			/* could not open directory */
			std::cerr << "ERROR: cannot read files in the idmap images directory" << std::endl;
			return 1;
		}

		orderStringsVector(filenames);
	}
	
	std::cout << std::endl;
	std::cout << "reassembling images" << std::endl;

	// start reassembling the images
	for(unsigned int i=0; i<filenames.size(); i++){
		if(i%100 == 0)
		{
			std::cout << "at image " << i << std::endl;
		}

		// copy the idmap in the structure
		std::string idname(idmap_folder_name);
		idname.append("/");
		idname.append(filenames[i]);

		cv::Mat mid = cv::imread(idname, cv::IMREAD_COLOR);

		for(unsigned int r=0; r<mid.rows; r++)
		{
			for(unsigned int c=0; c<mid.cols; c++)
			{
				idmap[r_idx[r] + c] = mid.at<cv::Vec3b>(r,c)[2];
			}
		}


		// copy the layers images in the structures
		for(unsigned int l=0; l<layers_folders_names.size(); l++)
		{
			std::string layname(layers_folders_names[l]);
			layname.append("/");
			layname.append(filenames[i]);

			cv::Mat mlay = cv::imread(layname, cv::IMREAD_COLOR);

			for(unsigned int r=0; r<mlay.rows; r++)
			{
				for(unsigned int c=0; c<mlay.cols; c++)
				{
					layers[l][r_idx[r] + c] = mlay.at<cv::Vec3b>(r,c)[2];
				}
			}
		}


		// merge the layers
		layers_merger.merge(idmap, layers, reconstructed);


		// save the reconstructed image
		cv::Mat reco(_rows, _cols, CV_16UC1);
		memcpy(reco.data, reconstructed, sizeof(unsigned short) * total_cells);

		std::string outname(_name_output_folder);
		outname.append("/");
		outname.append(filenames[i]);
		cv::imwrite(outname, reco);

	}


	return 0;
}
