

#include <mx/readColumns.hpp>
#include <mx/gslInterpolation.hpp>
using namespace mx;

int main()
{

   std::vector<double> x, r, g, b;

   std::vector<double> xt, rt, gt, bt;

   readColumns("bone_r.dat", r);
   readColumns("bone_g.dat", g);
   readColumns("bone_b.dat", b);
   
   x.resize(64);
   
   for(int i=0; i< 64; ++i)  x[i] = ((double)i)/63.*255.;
   
   rt.resize(256);
   gt.resize(256);
   bt.resize(256);
   
   xt.resize(256);
   for(int i=0; i< 256; ++i) xt[i] = i;
   
   
   mx::gsl_interpolate( gsl_interp_cspline, x, r, xt, rt);
   mx::gsl_interpolate( gsl_interp_cspline, x, g, xt, gt);
   mx::gsl_interpolate( gsl_interp_cspline, x, b, xt, bt);
   

   for(int i=0; i< 256; ++i)
   {
       std::cout << "qim->setColor(i++, qRgb(    " << (int) (rt[i]+0.5) << ",      " << (int) (gt[i]+0.5) << ",  " << (int) (bt[i]+0.5) << " ));\n";
   }
}
