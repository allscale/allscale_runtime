#include <stdio.h>
#include <math.h>


namespace allscale { namespace components { namespace util {


typedef struct HSV_color {
   double h;
   double s;
   double v;
} myHSV_Color;

typedef struct RGB_color {
   unsigned r;
   unsigned g;
   unsigned b;
} myRGB_Color;



myHSV_Color gradient_min, gradient_max;


myHSV_Color rgb_to_hsv(unsigned r, unsigned g, unsigned b) {

    myHSV_Color myColor;
    unsigned maxc, minc, diff;
    double h, s, v;

    if(r >= g) maxc = r;
    else maxc = g;

    if(b >= maxc) maxc = b;

    if(r <= g) minc = r;
    else minc = g;

    if(b <= minc) minc = b;

    v = (double)maxc;
    if(minc == maxc) {
       myColor.h = 0;
       myColor.s = 0;
       myColor.v = v;
       return myColor;
    }

    diff = maxc - minc;
    s = (double)diff / (double)maxc;
    double rc = (double)(maxc - r) / (double)diff;
    double gc = (double)(maxc - g) / (double)diff;
    double bc = (double)(maxc - b) / (double)diff;

    if(r == maxc)
        h = bc - gc;
    else if(g == maxc)
        h = 2.0 + rc - bc;
    else
        h = 4.0 + gc - rc;
    h = fmod((h / 6.0), 1.0); //comment: this calculates only the fractional part of h/6

    myColor.h = h;
    myColor.s = s;
    myColor.v = v;
    return myColor;
}

void init_gradient_HSV_color() {

   gradient_min = rgb_to_hsv(255, 255, 0); // green converted to HSV
   gradient_max = rgb_to_hsv(255, 0, 0); // accordingly for blue 
}


myRGB_Color hsv_to_rgb(double h, double s, double v) {

    myRGB_Color myColor;
    unsigned i;
    double f, p, q, t;

    if (s == 0.0) {
       myColor.r = v;
       myColor.g = v;
       myColor.b = v;
       return myColor;
    }


    i = unsigned(floor(h*6.0)); //comment: floor() should drop the fractional part
    f = (h*6.0) - i;
    p = v*(1.0 - s);
    q = v*(1.0 - s*f);
    t = v*(1.0 - s*(1.0 - f));

    if( i%6 == 0 ) {
       myColor.r = (unsigned)v;
       myColor.g = (unsigned)t;
       myColor.b = (unsigned)p;
       return myColor;
    }
    if( i == 1 ) {
       myColor.r = (unsigned)q;
       myColor.g = (unsigned)v;
       myColor.b = (unsigned)p;
       return myColor;
    }

    if( i == 2 ) {
       myColor.r = (unsigned)p;
       myColor.g = (unsigned)v;
       myColor.b = (unsigned)t;
       return myColor;
    }

    if( i == 3 ) {
       myColor.r = (unsigned)p;
       myColor.g = (unsigned)q;
       myColor.b = (unsigned)v;
       return myColor;
    }

    if( i == 4 ) {
       myColor.r = (unsigned)t;
       myColor.g = (unsigned)p;
       myColor.b = (unsigned)v;
       return myColor;
    }

    if( i == 5 ) {
       myColor.r = (unsigned)v;
       myColor.g = (unsigned)p;
       myColor.b = (unsigned)q;
       return myColor;
    }
    //comment: 0 <= i <= 6, so we never come here
}

double HSV_transition(double value, double maximum, double start_point, double end_point) {
    return (start_point + (end_point - start_point)*value/maximum);
}



myHSV_Color HSV_transition3(double value, double maximum, myHSV_Color min_gradient, myHSV_Color max_gradient) {

    myHSV_Color c;

    c.h= HSV_transition(value, maximum, min_gradient.h, max_gradient.h);
    c.s= HSV_transition(value, maximum, min_gradient.s, max_gradient.s);
    c.v= HSV_transition(value, maximum, min_gradient.v, max_gradient.v);
    return c;
}

unsigned
intensity_to_rgb(double value, double max) {

   myHSV_Color c1;
   myRGB_Color c2;

   c1 = HSV_transition3(value, max, gradient_min, gradient_max);
   c2 = hsv_to_rgb(c1.h, c1.s, c1.v);

//   c2 = RGB_transition3(value, max, RGB_min, RGB_max);
   return (c2.r << 16 | c2.g << 8 | c2.b);
}

}}}
