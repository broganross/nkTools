/* DisparityDistort.cpp

The MIT License (MIT)

Copyright (c) [2013] [Brogan Ross]

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

static const char* CLASS = "DisparityDistort";
static const char* HELP= "Distorts an image by the disparity channel";

#include "DDImage/NukeWrapper.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/Tile.h"
#include "DDImage/DDMath.h"
#include "DDImage/Thread.h"
#include <stdio.h>

using namespace std;
using namespace DD::Image;

class DisparityDistort : public Iop
{
	float _maxValue;
	float _minValue;
	bool _firstTime;
	Lock _lock;
	Channel dispChans[4];

public:
	int maximum_inputs() const {return 1; }
	int minimum_inputs() const {return 1; }

	DisparityDistort(Node* node) : Iop(node)
	{
		_maxValue = 0.0;
		_minValue = 1.0;
		_firstTime = true;
		dispChans[0] = Chan_Stereo_Disp_Left_X;
		dispChans[1] = Chan_Stereo_Disp_Left_Y;
		dispChans[2] = Chan_Stereo_Disp_Right_X;
		dispChans[3] = Chan_Stereo_Disp_Right_Y;
	}

	const char* Class() const {return CLASS; }
	const char* node_help() const {return HELP;}
	static const Op::Description d;

	void _open(){
		_firstTime = true;
	}

	// finds the maximum/minimum values in the disparity channel
	// @todo: adjust to get x max/min and y max/min
	void findMaxMin(){
		Guard guard(_lock);
		if (_firstTime){
			Format format = input0().format();
			const int fx = format.x();
			const int fy = format.y();
			const int fr = format.r();
			const int ft = format.t();
			ChannelSet dchan;
			for (int i=0; i < 4; i++){
				dchan += dispChans[i];
			}
			Interest interest(input0(), fx, fy, fr, ft, dchan, true);
			interest.unlock();
			for (int y=fy; y < ft; y++){
				progressFraction(y, ft - fy);
				Row row(fx, fr);
				if (aborted())
					return;
				foreach (z, dchan)
				{
					row.get(input0(), y, fx, fr, z);
					const float *cur = row[z] + fx;
					const float *end = row[z] + fr;
					while (cur < end){
						_maxValue = std::max((float)*cur, _maxValue);
						_minValue = std::min((float)*cur, _minValue);
						cur++;
					}
				}
			}
			_firstTime = false;
		}
	}

	void _validate(bool for_real){
		std::cout << "validate" << std::endl;
		copy_info();
		float mst = std::max(fabs(_maxValue), fabs(_minValue));
		info_.y(info_.y() - mst);
		info_.t(info_.t() + mst);
		info_.x(info_.x() - mst);
		info_.r(info_.r() + mst);
		set_out_channels(Mask_All);
		std::cout << info_.x() << " " << info_.r() << std::endl;
	}

	void in_channels(int, ChannelSet &m) const {
		for (int i=0; i < 4; i++){
			m += dispChans[i];
		}
	}

	void _request(int x, int y, int r, int t, ChannelMask channels, int count){
		std::cout << "request" << std::endl;
		ChannelSet cl(channels);
		in_channels(0, cl);
		float mst = std::max(fabs(_maxValue), fabs(_minValue));
		x -= mst;
		r += mst;
		y -= mst;
		t += mst;
		input0().request(x, y, r, t, cl, count);
		std::cout << x << " " << y << " " << r << " " << t << std::endl;
	}

	void engine(int y, int x, int r, ChannelMask channels, Row& out)
	{
		ChannelSet cl(channels);
		in_channels(0, cl);
		findMaxMin();
		if (aborted())
			return;
		float mst = std::max(fabs(_maxValue), fabs(_minValue));
		Row in(info_.x()-mst, info_.r()-mst);
		if (aborted())
			return;

		in.get(input0(), y, in.getLeft(), in.getRight(), cl);

		for (int i=0; i < 4; i++)
		{
			if (!intersect(in.writable_channels(), dispChans[i]))
				dispChans[i] = Chan_Black;
		}

		const float* dispLx = in[dispChans[0]];
//		const float* dispLy = in[dispChans[1]];
		const float* dispRx = in[dispChans[2]];
//		const float* dispRy = in[dispChans[3]];
		std::cout << "in  row: " << in.getLeft() << " " << in.getRight() << std::endl;
		foreach(z, channels){
			out.erase(z);
			float* to = out.writable(z);
			const float* from = in[z];
			for (int X=x; X<r; X++){
				int lval = int(dispLx[X] + .5);
				int rval = int(dispRx[X] + .5);
//				std::cout << "in  row: " << in.getLeft() << " " << in.getRight() << std::endl;
//				std::cout << "out row: " << out.getLeft() << " " << out.getRight() << std::endl;
//				std::cout << "X: " << X;
//				std::cout << " lval: " << lval;
//				std::cout <<" To: " << X + lval << std::endl;
//				std::cout << "From: " << from[X];
//				std::cout << " To: " << to[X + lval] << std::endl;

//				to[X + lval] = from[X];
				to[X] = from[X];
			}
		}
	}

};
static Op* build(Node* node) { return new NukeWrapper(new DisparityDistort(node)); }
const Op::Description DisparityDistort::d(CLASS, "Filter/DisparityDistort", build);

// end of DisparityDistort.cpp
