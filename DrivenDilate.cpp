/* DrivenDilate.C
Based on The Foundry's code for a Dilate.cpp

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

 static const char* CLASS = "DrivenDilate";

static const char* HELP =
  "Box Morphological Filter\n\n"
  "Maximum (or minimum) of a rectangular area around each pixel. This "
  "can be used to grow or shrink mattes.";

#include "DDImage/NukeWrapper.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/Tile.h"
#include "DDImage/DDMath.h"
#include "DDImage/Thread.h"
#include <stdio.h>
#include <typeinfo>

using namespace std;
using namespace DD::Image;

class DrivenDilate : public Iop
{
  // Where the knob stores it's values:
    double w, h;
    int bboxAdjust;
    float _maxValue;
    bool _firstTime;
    Lock _lock;
	Channel maskChan[1];
	int h_size;
	int h_do_min;
	int v_size;
	int v_do_min;

public:
	int maximum_inputs() const { return 1; }
	int minimum_inputs() const { return 1; }

	DrivenDilate(Node* node) : Iop(node)
	{
		w = h = 0;
		bboxAdjust = 0;
		maskChan[0] = Chan_Black;
		_maxValue = 0.0;
		_firstTime = true;
	}

	const char* Class() const { return CLASS; }
	const char* node_help() const { return HELP; }

    void knobs(Knob_Callback f)
    {
    	Input_Channel_knob( f, maskChan, 1, 0, "maskChannel", "Mask Channel");
    	WH_knob(f, &w, IRange(-100, 100), "size");
    	Int_knob(f, &bboxAdjust, "bbox");
    }

    static const Op::Description d;

	void _validate(bool for_real)
	{
		h_size = int(fabs(w) + .5);
		h_do_min = w < 0;
		v_size = int(fabs(h) + .5);
		v_do_min = h < 0;
		copy_info();
		info_.y(info_.y() - (bboxAdjust + std::max(h_size, (int)(h_size * _maxValue))));
		info_.t(info_.t() + (bboxAdjust + std::max(v_size, (int)(v_size * _maxValue))));
		info_.x(info_.x() - (bboxAdjust + std::max(h_size, (int)(h_size * _maxValue))));
		info_.r(info_.r() + (bboxAdjust + std::max(v_size, (int)(v_size * _maxValue))));
		set_out_channels(h_size || v_size ? Mask_All : Mask_None);
	}

	void _request(int x, int y, int r, int t, ChannelMask channels, int count)
	{
		ChannelSet cl(channels);
		in_channels(0, cl);

		x -= bboxAdjust + std::max(h_size, (int)(h_size * _maxValue));
		r += bboxAdjust + std::max(h_size, (int)(h_size * _maxValue));
		y -= bboxAdjust + std::max(v_size, (int)(v_size * _maxValue));
		t += bboxAdjust + std::max(v_size, (int)(v_size * _maxValue));
		input0().request(x, y, r, t, cl, count);
	}

	void in_channels(int, ChannelSet &m) const{
		m += maskChan[0];
	}

  // Find the minimum of all the input rows:
	void get_vpass(int y, int x, int r, ChannelMask channels, Row& out)
	{
		if (!v_size) {
		  input0().get(y, x, r, channels, out);
		  return;
		}
		Channel mchan(maskChan[0]);

		// determine max/min sizes of the tile
		float tm = y - (v_size * _maxValue);
		if (tm < info_.y())
			tm = info_.y();
		float tx = y + (v_size*_maxValue);
		if (tx > info_.t())
			tx = info_.t();
		Tile tile(input0(), x, tm, r, tx + 1, channels);
		if (aborted())
			return;

		if (!intersect(tile.channels(), mchan))
			mchan = Chan_Black;
		foreach (z, channels) {
			if (z == mchan)
				continue;
			float* TO = out.writable(z);
			int X;
			int Y = tile.y();
			for (X = tile.x(); X < tile.r(); X++)
				TO[X] = tile[z][Y][X];
			// get vertical values
			for (X=tile.x(); X < tile.r(); X++){
				float mval = tile[mchan][y][X];
				int start = y - (v_size*mval);
				if (start < tile.y())
					start = tile.y();
				int end = y + (v_size*mval);
				if (end > tile.t())
					end = tile.t();

				for (int Y=start; Y < end; Y++){
					if (v_do_min){
						if (tile[z][Y][X] < TO[X])
							TO[X] = tile[z][Y][X];
					} else {
						if (tile[z][Y][X] > TO[X])
							TO[X] = tile[z][Y][X];
					}
				}
			}
			// get horizontal values
/*			if (h_size){
				for (X=tile.x(); X < tile.r(); X++){
					float mval = tile[mchan][y][X];
					int np = X - (int)(mval);
					int pp = X + (int)(mval);
					float v = TO[X];
					for (int cx=np; cx < pp; cx++){
						if (h_do_min){
							v = MIN(v, TO[cx]);
						} else {
							v = MAX(v, TO[cx]);
						}
					}
					TO[X] = v;
				}
			}*/
			// pad the ends that go outside the source:
			for (X = x; X < tile.x(); X++)
				TO[X] = TO[tile.x()];
			for (X = tile.r(); X < r; X++)
				TO[X] = TO[tile.r() - 1];
		}
	}

	void _open(){
		_firstTime = true;
	}

	/*! Finds the maximum and minimum pixel value for the frame in the mask channel
	 */
	void findMaxMin(){
		Guard guard(_lock);
		if (_firstTime){
			Format format = input0().format();
			const int fx = format.x();
			const int fy = format.y();
			const int fr = format.r();
			const int ft = format.t();
			Channel mchan(*maskChan);
			Interest interest(input0(), fx, fy, fr, ft, mchan, true);
			interest.unlock();
			_maxValue = 0.0;
			for (int y = fy; y < ft; y++){
				progressFraction(y, ft - fy);
				Row row(fx, fr);
				row.get(input0(), y, fx, fr, mchan);
				if (aborted())
					return;


				const float *cur = row[mchan] + fx;
				const float *end = row[mchan] + fr;
				while (cur < end){
					_maxValue = std::max((float)fabs((float)*cur), _maxValue);
					cur++;
				}
			}
			_firstTime = false;
		}
	}

	// The engine does the horizontal minimum pass:
	void engine(int y, int x, int r, ChannelMask channels, Row& out)
	{
		ChannelSet cl(channels);
		in_channels(0, cl);

		findMaxMin();
		if (aborted())
			return;

		if (h_size){
			// determine max/min row size
			float rm = x - (bboxAdjust + (h_size * _maxValue));
			if (rm < info_.x())
				rm = info_.x();
			float rx = r + (bboxAdjust + (h_size * _maxValue));
			if (rx > info_.r())
				rx = info_.r();

			Row in(rm, rx);
			get_vpass(y, rm, rx, cl, in);
			if (aborted())
				return;

			Channel mchan = maskChan[0];
			if (!intersect(in.writable_channels(), mchan))
				mchan = Chan_Black;

			const float* DRIVEN = in[mchan];
			foreach (z, cl){
				if (z == mchan)
					continue;
				float* TO = out.writable(z);
				const float* FROM = in[z];
				int X;
				for (X=x; X < r; X++){
					float mval = DRIVEN[X];
					int np = (X - (int)(h_size * mval));
					int pp = (X + (int)(h_size * mval));
					std::cout << y << " " << X << std::endl;
					float v = FROM[X];
					for (int cx=np; cx < pp; cx++){
						if (h_do_min){
							v = MIN(v, FROM[cx]);
						} else {
							v = MAX(v, FROM[cx]);
						}
					}
					TO[X] = v;
				}
			}
/*
			Channel mchan = maskChan[0];
			if (!intersect(in.writable_channels(), mchan))
				mchan = Chan_Black;
			const float* DRIVEN = in[mchan];
			foreach(z, cl){
				if (z == mchan)
					continue;
				float* TO = out.writable(z);
				int X;
				const float* FROM = in[z];
				float mv = DRIVEN[x];
				int length = (int)((h_size * mv) * 2);
				float v = FROM[x + (int)(h_size * mv)];
				for (X=x; X < r; X++){
					mv = DRIVEN[X];
//					length = (h_size * mv) * 2;
					int p = X + (int)(h_size*mv);
					if (h_do_min){
						v = MIN(v, FROM[p]);
					} else {
						v = MAX(v, FROM[p]);
					}
					TO[X] = v;
				}
			    X = in.getRight();
			    mv = DRIVEN[X];
			    v = FROM[X - 1 - (int)(h_size * mv)];
			    for (; X > x; X--) {
			    	mv = DRIVEN[X];
			    	int p = X - (int)(h_size*mv);
			    	if (h_do_min){
			    		v = MIN(v, FROM[p]);
			    	} else {
			    		v = MAX(v, FROM[p]);
			    	}
			    	TO[X] = v;
			    }
			}
*/
		} else {
			get_vpass(y, x, r, cl, out);
			if (aborted())
				return;
		}
/*		if (h_size) {
			Row in(x - (h_size*_maxValue), r + (h_size*_maxValue));
			get_vpass(y, x - (h_size*_maxValue), r + (h_size*_maxValue), cl, in);
			if (aborted())
				return;
			const int length = 2 * h_size;
			foreach (z, channels) {
				const float* mchan = in[maskChan[0]];
				const float* FROM = in[z];
				float* TO = out.writable(z);
				float mv = mchan[x] * h_size;
				float v = FROM[x + h_size];
				if (h_do_min) {
					int X;
					for (X = x; X < r; X++) {
						if ((X - x) % length)
							v = MIN(v, FROM[X + h_size]);
						else
							v = FROM[X + h_size];
						TO[X] = v;
					}
					// we need to round up start to next multiple of length:
					X = (r - x) % length;
					if (X < 0)
						X = -X;
					X = X ? r + length - X : r;
					v = FROM[X - 1 - h_size];
					while (X > r) { --X;
                          v = MIN(v, FROM[X - 1 - h_size]); }
					for (; X > x; X--) {
						if ((X - x) % length)
							v = MIN(v, FROM[X - 1 - h_size]);
						else
							v = FROM[X - 1 - h_size];
						TO[X - 1] = MIN(TO[X - 1], v);
					}
				}
				else {
					int X;
					for (X = x; X < r; X++) {
						if ((X - x) % length)
							v = MAX(v, FROM[X + h_size]);
						else
							v = FROM[X + h_size];
						TO[X] = v;
					}
					// we need to round up start to next multiple of length:
					X = (r - x) % length;
					if (X < 0)
						X = -X;
					X = X ? r + length - X : r;
					v = FROM[X - 1 - h_size];
					while (X > r) { --X;
								  v = MAX(v, FROM[X - 1 - h_size]); }
					for (; X > x; X--) {
						if ((X - x) % length)
							v = MAX(v, FROM[X - 1 - h_size]);
						else
							v = FROM[X - 1 - h_size];
						TO[X - 1] = MAX(TO[X - 1], v);
					}
				}
			}
    }
	else {
		get_vpass(y, x, r, cl, out);
	}
	*/
}


};

static Op* construct(Node* node) { return new NukeWrapper(new DrivenDilate(node)); }
const Op::Description DrivenDilate::d(CLASS, construct);

// end of DrivenDilate.C
