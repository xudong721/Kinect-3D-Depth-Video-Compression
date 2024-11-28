// DSL - Depth Streaming Library, a software for streaming depth images sequences
// Copyright (C) 2014 Fabrizio Nenci and Luciano Spinello and Cyrill Stachniss

// This file is part of DSL

// DSL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// DSL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with DSL.  If not, see <http://www.gnu.org/licenses/>.

#include "layers_merger.h"
#include <string.h>	// memcpy
#include <stdlib.h>	// malloc

ds::LayersMerger::LayersMerger(std::vector< std::pair<unsigned short, unsigned short> > & intervals, unsigned int rows, unsigned int cols) :
	_rows(rows) , _cols(cols) , _totalCells(_rows * _cols), _outputLength(sizeof(unsigned short) * _totalCells)
{
	this->prepareLookupValues(intervals);
}


ds::LayersMerger::LayersMerger(ds::LayersMerger & lm) :
	_rows(lm._rows) , _cols(lm._cols) , _totalCells(_rows * _cols), _outputLength(sizeof(unsigned short) * _totalCells)
{
	for(unsigned int i=0; i<lm._lookupValues.size(); i++)
	{
		unsigned short * lt = static_cast<unsigned short *>(malloc(sizeof(unsigned short) * 256));
		memcpy(lt, lm._lookupValues[i], sizeof(unsigned short) * 256);
		_lookupValues.push_back(lt);
	}
}


ds::LayersMerger::~LayersMerger()
{
	for(unsigned int i=0; i<_lookupValues.size(); i++)
	{
		free(_lookupValues[i]);
	}
}



void ds::LayersMerger::prepareLookupValues(std::vector< std::pair<unsigned short, unsigned short> > & intervals)
{
	for(unsigned int i=0; i<intervals.size(); i++)
	{
		unsigned short span = intervals[i].second - intervals[i].first;
		unsigned short * lookup_table = static_cast<unsigned short *>(malloc(sizeof(unsigned short) * 256));
		for(unsigned int v=0; v<256; v++)
		{
			double value = v;
			value = value/255.0;
			value = (value * span) + intervals[i].first;
			
			lookup_table[v] = value + 0.5;	// the +0.5 is for rounding to the closer integer, as the cast to short will cut away the decimal part
		}
		_lookupValues.push_back(lookup_table);
	}
}



void ds::LayersMerger::merge(unsigned char * idmap, std::vector< unsigned char * > & layers, unsigned short * output)
{
	memset(output, 0, _outputLength);

	for(unsigned int i=0; i<_totalCells; i++){
		unsigned char l = idmap[i];
		// std::cout << "l:\t" << (int)l << std::endl;
		if(l >= layers.size()) continue;
		

		unsigned char value = layers[l][i];
		// std::cout << "\tvalue:\t" << (int)value << std::endl;

		output[i] = _lookupValues[l][value];
	}
}
