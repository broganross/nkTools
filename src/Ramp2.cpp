/* Ramp2.cpp
 Creates a gradient from one point to another with color
 controls for both points

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


static const char* const CLASS="Ramp2";
static const char* const HELP="A basic gradient ramp with two points and two color values";

#include "DDImage/ColorLookup.h"
#include "DDImage/DDWindows.h"
#include "DDImage/Iop.h"
#include "DDImage/Format.h"
#include "DDImage/ViewerContext.h"
#include "DDImage/gl.h"
#include "DDImage/Knobs.h"
#include "DDImage/Knob.h"
#include "DDImage/Pixel.h"
#include "DDImage/Row.h"
#include "DDImage/DDMath.h"
#include "DDImage/Transform.h"
#include "DDImage/LookupCurves.h"
#include <math.h>
#include "DDImage/Vector2.h"

using namespace DD::Image;
using namespace std;

class Ramp2 : public Iop
{
    Vector2 p0;
    Vector2 p1;
    float low_col[4];
    float hi_col[4];
    Channel channel[4];
    Vector2 rV; // vector of the ramp
    Vector2 vP0; // p0 vector
    Vector2 vP1; // p1 vector
    FormatPair formats;
    float ca;
    float sa;

public:
    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Description desc;
    const char* displayname() const { return "Ramp2"; }

    Ramp2(Node* node) : Iop(node)
    {
        inputs(0);
        ca = cos(90.0 * (3.14/180));
        sa = sin(90.0 * (3.14/180));
        p0.x = 500;
        p0.y = 0;
        p1.x = 500;
        p1.y = 500;
        low_col[0] = low_col[1] = low_col[2] = low_col[3] = 0;
        hi_col[0] = hi_col[1] = hi_col[2] = hi_col[3] = 1;

        channel[0] = Chan_Red;
        channel[1] = Chan_Green;
        channel[2] = Chan_Blue;
        channel[3] = Chan_Alpha;
        formats.format(0);

        Vector2 rV = Vector2(0.0, 0.0); //
        Vector2 vP0 = Vector2(0.0, 0.0);
        Vector2 vP1 = Vector2(0.0, 0.0);
    };

    void _validate(bool for_real)
    {
        bool non_zero = false;
        info_.black_outside(false);
        ChannelSet tchan;
        for (int i=0; i<4; i++)
        {
            info_.turn_on(channel[i]);
        }

        // set format and bbox
        info_.full_size_format(*formats.fullSizeFormat());
        info_.format(*formats.format());
        info_.set(format());
        info_.x(std::min(std::min(float(info_.x()), p0.x), p1.x) + 1.0);
        info_.y(std::min(std::min(float(info_.y()), p0.y), p1.y) + 1.0);
        info_.r(std::max(std::max(float(info_.r()), p0.x), p1.x) + 1.0);
        info_.t(std::max(std::max(float(info_.t()), p0.y), p1.y) + 1.0);

        if (for_real)
        {
            vP0 = Vector2(p0.x, p0.y); 
            vP1 = Vector2(p1.x, p1.y);
            rV = Vector2((p1.x - p0.x), (p1.y - p0.y));
        }
    }

    void engine(int y, int xx, int r, ChannelMask channels, Row& row)
    {
        for (int x=xx; x<r; x++)
        {
            for (int z=0; z<4; z++){
                if (low_col[z] || hi_col[z]){
                    float* out = row.writable(channel[z]);
                    Vector2 vC = Vector2(x, y); // current pixel
                    // dot products
                    float c1 = rV.x * p0.x + rV.y * p0.y;
                    float c2 = rV.x * p1.x + rV.y * p1.y;
                    float c = rV.x * x + rV.y * y;
                    float pixcol = (low_col[z] * (c2 - c) + hi_col[z] * (c - c1))/(c2 - c1);

                    out[x] = clamp(pixcol, min(low_col[z], hi_col[z]), max(low_col[z], hi_col[z]));
                }
                continue;
            }
        }
    }

    void knobs(Knob_Callback f)
    {
        Format_knob(f, &formats, "format");
        Tooltip(f, "Image format for this node.");
        Obsolete_knob(f, "full_format", "knob format $value");
        Divider(f);
        AColor_knob(f, low_col, "col0", "color 0");
        Tooltip(f, "Color associated to p0");
        AColor_knob(f, hi_col, "col1", "color 1");
        Tooltip(f, "Color associated to p1");
        XY_knob(f, &p0[0], "p0");
        Tooltip(f, "Position of p0");
        XY_knob(f, &p1[0], "p1");
        Tooltip(f, "Position of p1");
        Newline(f);
    }

    void build_handles(ViewerContext* ctx)
    {
        build_knob_handles(ctx);
        if (ctx->transform_mode() != VIEWER_2D)
            return;
        add_draw_handle(ctx);
    }

    void find_points(float x, float y, float r, float t, Vector2 &v0, Vector2 &v1)
    {
        if (y > (float)info_.format().r()){
            v0.x = (float)info_.format().r();
            v0.y = r;
        } else if (y < (float)info_.format().x()){
            v0.x = (float)info_.format().x();
            v0.y = x;
        } else {
            v0.x = y;
            v0.y = (float)info_.format().x();
        };
        if (t > (float)info_.format().r()){
            v1.x = (float)info_.format().r();
            v1.y = r;
        } else if (t < (float)info_.format().x()){
            v1.x = (float)info_.format().x();
            v1.y = x;
        } else {
            v1.x = t;
            v1.y = (float)info_.format().t();
        };
    };

    void draw_handle(ViewerContext* ctx)
    {
        if (!ctx->draw_lines())
            return;

        validate(false);
        glDisable(GL_LINE_STIPPLE);
        glColor(ctx->fg_color());
        glLineWidth(1.5f);

        // draw the ramp line
        glBegin(GL_LINES);
        glVertex2d(p0.x, p0.y);
        glVertex2d(p1.x, p1.y);
        glEnd();

        // make the perpendicular vectors
        double x = p0.x + ((rV.x*ca) - (rV.y*sa));
        double y = p0.y + ((rV.x*sa) + (rV.y*ca));
        double a = p1.x + ((rV.x*ca) - (rV.y*sa));
        double b = p1.y + ((rV.x*sa) + (rV.y*ca));
        Vector2 pp0 = Vector2(x, y);
        Vector2 pp1 = Vector2(a, b);

        // find the intercepts with the display window
        float sl = ((p0.y-pp0.y)/(p0.x-pp0.x));
        float xInt0 = -((sl * pp0.x) - pp0.y);
        float xInt1 = -((sl * pp1.x) - pp1.y);
        float rInt0 = (sl * info_.format().r()) + xInt0;
        float rInt1 = (sl * info_.format().r()) + xInt1;
        float yInt0 = (info_.format().y() - xInt0)/sl;
        float yInt1 = (info_.format().y() - xInt1)/sl;
        float tInt0 = (info_.format().t() - xInt0)/sl;
        float tInt1 = (info_.format().t() - xInt1)/sl;

        Vector2 v1 = Vector2(0.0, 0.0);
        Vector2 v2 = Vector2(0.0, 0.0);
        find_points(xInt0, yInt0, rInt0, tInt0, v1, v2);

        glLineStipple(1, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);
        glColor(ctx->fg_color());
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2d(v1.x, v1.y);
        glVertex2d(v2.x, v2.y);
        glEnd();

        find_points(xInt1, yInt1, rInt1, tInt1, v1, v2);

        glLineStipple(1, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);
        glColor(ctx->fg_color());
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2d(v1.x, v1.y);
        glVertex2d(v2.x, v2.y);
        glEnd();
    }

};

static Iop* constructor(Node* node) { return new Ramp2(node); }
const Iop::Description Ramp2::desc(CLASS, "Draw/Ramp2", constructor);