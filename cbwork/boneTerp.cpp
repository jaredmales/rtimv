

#include <mx/ioutils/readColumns.hpp>
#include <mx/gslInterpolation.hpp>
using namespace mx::ioutils;

int main()
{

   std::vector<double> x, r, g, b;

   std::vector<double> xt, rt, gt, bt;

   readColumns("bone_r.dat", r);
   readColumns("bone_g.dat", g);
   readColumns("bone_b.dat", b);
   
   x.resize(64);
   
   for(int i=0; i< 64; ++i)  x[i] = ((double)i)/63.*253.;
   
   rt.resize(254);
   gt.resize(254);
   bt.resize(254);
   
   xt.resize(254);
   for(int i=0; i< 254; ++i) xt[i] = i;
   
   
   mx::gsl_interpolate( gsl_interp_cspline, x, r, xt, rt);
   mx::gsl_interpolate( gsl_interp_cspline, x, g, xt, gt);
   mx::gsl_interpolate( gsl_interp_cspline, x, b, xt, bt);
   

   int i;
   for(i=0; i< 253; ++i)
   {
       std::cout << "qim->setColor(i++, qRgb(    " << (int) (rt[i]+0.5) << ",      " << (int) (gt[i]+0.5) << ",  " << (int) (bt[i]+0.5) << " ));\n";
   }
   std::cout << "qim->setColor(i,   qRgb(    " << (int) (rt[i]+0.5) << ",      " << (int) (gt[i]+0.5) << ",  " << (int) (bt[i]+0.5) << " ));\n";
}
