#ifndef colorMaps_hpp
#define colorMaps_hpp

struct rtimvColorMap
{
    std::string m_helpFontColor{ "#3DA5FF" };
    std::string m_gageFontColor{ "#3DA5FF" };
    std::string m_zoomFontColor{ "#3DA5FF" };
    std::string m_statusFontColor{ "#3DA5FF" };
    std::string m_statsBoxColor{ "#3DA5FF" };
    std::string m_userShapeColor{ "lime" };

    std::string m_warningFontColor{ "lightgreen" };
    std::string m_saveBoxFontColor{ "lime" };
    std::string m_loopFontColor{ "lime" };
};

/// Jet: load the Matlab Jet colormap
inline int load_colorbar_jet( QImage *qim )
{
    int i = 0;
#if 1
    // The jet color table interpolated with Steffen's method onto 253 colors
    qim->setColor( i++, qRgb( 0, 0, 144 ) );
    qim->setColor( i++, qRgb( 0, 0, 148 ) );
    qim->setColor( i++, qRgb( 0, 0, 152 ) );
    qim->setColor( i++, qRgb( 0, 0, 156 ) );
    qim->setColor( i++, qRgb( 0, 0, 160 ) );
    qim->setColor( i++, qRgb( 0, 0, 164 ) );
    qim->setColor( i++, qRgb( 0, 0, 168 ) );
    qim->setColor( i++, qRgb( 0, 0, 172 ) );
    qim->setColor( i++, qRgb( 0, 0, 176 ) );
    qim->setColor( i++, qRgb( 0, 0, 180 ) );
    qim->setColor( i++, qRgb( 0, 0, 184 ) );
    qim->setColor( i++, qRgb( 0, 0, 188 ) );
    qim->setColor( i++, qRgb( 0, 0, 192 ) );
    qim->setColor( i++, qRgb( 0, 0, 196 ) );
    qim->setColor( i++, qRgb( 0, 0, 200 ) );
    qim->setColor( i++, qRgb( 0, 0, 204 ) );
    qim->setColor( i++, qRgb( 0, 0, 208 ) );
    qim->setColor( i++, qRgb( 0, 0, 212 ) );
    qim->setColor( i++, qRgb( 0, 0, 216 ) );
    qim->setColor( i++, qRgb( 0, 0, 220 ) );
    qim->setColor( i++, qRgb( 0, 0, 224 ) );
    qim->setColor( i++, qRgb( 0, 0, 228 ) );
    qim->setColor( i++, qRgb( 0, 0, 232 ) );
    qim->setColor( i++, qRgb( 0, 0, 236 ) );
    qim->setColor( i++, qRgb( 0, 0, 240 ) );
    qim->setColor( i++, qRgb( 0, 0, 244 ) );
    qim->setColor( i++, qRgb( 0, 0, 249 ) );
    qim->setColor( i++, qRgb( 0, 0, 253 ) );
    qim->setColor( i++, qRgb( 0, 0, 255 ) );
    qim->setColor( i++, qRgb( 0, 1, 255 ) );
    qim->setColor( i++, qRgb( 0, 5, 255 ) );
    qim->setColor( i++, qRgb( 0, 11, 255 ) );
    qim->setColor( i++, qRgb( 0, 15, 255 ) );
    qim->setColor( i++, qRgb( 0, 19, 255 ) );
    qim->setColor( i++, qRgb( 0, 23, 255 ) );
    qim->setColor( i++, qRgb( 0, 27, 255 ) );
    qim->setColor( i++, qRgb( 0, 31, 255 ) );
    qim->setColor( i++, qRgb( 0, 35, 255 ) );
    qim->setColor( i++, qRgb( 0, 39, 255 ) );
    qim->setColor( i++, qRgb( 0, 43, 255 ) );
    qim->setColor( i++, qRgb( 0, 47, 255 ) );
    qim->setColor( i++, qRgb( 0, 51, 255 ) );
    qim->setColor( i++, qRgb( 0, 55, 255 ) );
    qim->setColor( i++, qRgb( 0, 59, 255 ) );
    qim->setColor( i++, qRgb( 0, 63, 255 ) );
    qim->setColor( i++, qRgb( 0, 67, 255 ) );
    qim->setColor( i++, qRgb( 0, 71, 255 ) );
    qim->setColor( i++, qRgb( 0, 75, 255 ) );
    qim->setColor( i++, qRgb( 0, 79, 255 ) );
    qim->setColor( i++, qRgb( 0, 83, 255 ) );
    qim->setColor( i++, qRgb( 0, 87, 255 ) );
    qim->setColor( i++, qRgb( 0, 91, 255 ) );
    qim->setColor( i++, qRgb( 0, 95, 255 ) );
    qim->setColor( i++, qRgb( 0, 99, 255 ) );
    qim->setColor( i++, qRgb( 0, 103, 255 ) );
    qim->setColor( i++, qRgb( 0, 107, 255 ) );
    qim->setColor( i++, qRgb( 0, 111, 255 ) );
    qim->setColor( i++, qRgb( 0, 115, 255 ) );
    qim->setColor( i++, qRgb( 0, 119, 255 ) );
    qim->setColor( i++, qRgb( 0, 123, 255 ) );
    qim->setColor( i++, qRgb( 0, 127, 255 ) );
    qim->setColor( i++, qRgb( 0, 131, 255 ) );
    qim->setColor( i++, qRgb( 0, 135, 255 ) );
    qim->setColor( i++, qRgb( 0, 139, 255 ) );
    qim->setColor( i++, qRgb( 0, 143, 255 ) );
    qim->setColor( i++, qRgb( 0, 147, 255 ) );
    qim->setColor( i++, qRgb( 0, 151, 255 ) );
    qim->setColor( i++, qRgb( 0, 155, 255 ) );
    qim->setColor( i++, qRgb( 0, 159, 255 ) );
    qim->setColor( i++, qRgb( 0, 163, 255 ) );
    qim->setColor( i++, qRgb( 0, 167, 255 ) );
    qim->setColor( i++, qRgb( 0, 171, 255 ) );
    qim->setColor( i++, qRgb( 0, 175, 255 ) );
    qim->setColor( i++, qRgb( 0, 179, 255 ) );
    qim->setColor( i++, qRgb( 0, 183, 255 ) );
    qim->setColor( i++, qRgb( 0, 187, 255 ) );
    qim->setColor( i++, qRgb( 0, 191, 255 ) );
    qim->setColor( i++, qRgb( 0, 195, 255 ) );
    qim->setColor( i++, qRgb( 0, 199, 255 ) );
    qim->setColor( i++, qRgb( 0, 203, 255 ) );
    qim->setColor( i++, qRgb( 0, 207, 255 ) );
    qim->setColor( i++, qRgb( 0, 211, 255 ) );
    qim->setColor( i++, qRgb( 0, 215, 255 ) );
    qim->setColor( i++, qRgb( 0, 219, 255 ) );
    qim->setColor( i++, qRgb( 0, 223, 255 ) );
    qim->setColor( i++, qRgb( 0, 227, 255 ) );
    qim->setColor( i++, qRgb( 0, 231, 255 ) );
    qim->setColor( i++, qRgb( 0, 235, 255 ) );
    qim->setColor( i++, qRgb( 0, 239, 255 ) );
    qim->setColor( i++, qRgb( 0, 243, 255 ) );
    qim->setColor( i++, qRgb( 0, 248, 255 ) );
    qim->setColor( i++, qRgb( 0, 252, 255 ) );
    qim->setColor( i++, qRgb( 0, 255, 255 ) );
    qim->setColor( i++, qRgb( 1, 255, 254 ) );
    qim->setColor( i++, qRgb( 4, 255, 251 ) );
    qim->setColor( i++, qRgb( 9, 255, 246 ) );
    qim->setColor( i++, qRgb( 14, 255, 242 ) );
    qim->setColor( i++, qRgb( 18, 255, 238 ) );
    qim->setColor( i++, qRgb( 22, 255, 234 ) );
    qim->setColor( i++, qRgb( 26, 255, 230 ) );
    qim->setColor( i++, qRgb( 30, 255, 226 ) );
    qim->setColor( i++, qRgb( 34, 255, 222 ) );
    qim->setColor( i++, qRgb( 38, 255, 218 ) );
    qim->setColor( i++, qRgb( 42, 255, 214 ) );
    qim->setColor( i++, qRgb( 46, 255, 210 ) );
    qim->setColor( i++, qRgb( 50, 255, 206 ) );
    qim->setColor( i++, qRgb( 54, 255, 202 ) );
    qim->setColor( i++, qRgb( 58, 255, 198 ) );
    qim->setColor( i++, qRgb( 62, 255, 194 ) );
    qim->setColor( i++, qRgb( 66, 255, 190 ) );
    qim->setColor( i++, qRgb( 70, 255, 186 ) );
    qim->setColor( i++, qRgb( 74, 255, 182 ) );
    qim->setColor( i++, qRgb( 78, 255, 178 ) );
    qim->setColor( i++, qRgb( 82, 255, 174 ) );
    qim->setColor( i++, qRgb( 86, 255, 170 ) );
    qim->setColor( i++, qRgb( 90, 255, 166 ) );
    qim->setColor( i++, qRgb( 94, 255, 162 ) );
    qim->setColor( i++, qRgb( 98, 255, 158 ) );
    qim->setColor( i++, qRgb( 102, 255, 154 ) );
    qim->setColor( i++, qRgb( 106, 255, 150 ) );
    qim->setColor( i++, qRgb( 110, 255, 146 ) );
    qim->setColor( i++, qRgb( 114, 255, 142 ) );
    qim->setColor( i++, qRgb( 118, 255, 138 ) );
    qim->setColor( i++, qRgb( 122, 255, 134 ) );
    qim->setColor( i++, qRgb( 126, 255, 130 ) );
    qim->setColor( i++, qRgb( 130, 255, 126 ) );
    qim->setColor( i++, qRgb( 134, 255, 122 ) );
    qim->setColor( i++, qRgb( 138, 255, 118 ) );
    qim->setColor( i++, qRgb( 142, 255, 114 ) );
    qim->setColor( i++, qRgb( 146, 255, 110 ) );
    qim->setColor( i++, qRgb( 150, 255, 106 ) );
    qim->setColor( i++, qRgb( 154, 255, 102 ) );
    qim->setColor( i++, qRgb( 158, 255, 98 ) );
    qim->setColor( i++, qRgb( 162, 255, 94 ) );
    qim->setColor( i++, qRgb( 166, 255, 90 ) );
    qim->setColor( i++, qRgb( 170, 255, 86 ) );
    qim->setColor( i++, qRgb( 174, 255, 82 ) );
    qim->setColor( i++, qRgb( 178, 255, 78 ) );
    qim->setColor( i++, qRgb( 182, 255, 74 ) );
    qim->setColor( i++, qRgb( 186, 255, 70 ) );
    qim->setColor( i++, qRgb( 190, 255, 66 ) );
    qim->setColor( i++, qRgb( 194, 255, 62 ) );
    qim->setColor( i++, qRgb( 198, 255, 58 ) );
    qim->setColor( i++, qRgb( 202, 255, 54 ) );
    qim->setColor( i++, qRgb( 206, 255, 50 ) );
    qim->setColor( i++, qRgb( 210, 255, 46 ) );
    qim->setColor( i++, qRgb( 214, 255, 42 ) );
    qim->setColor( i++, qRgb( 218, 255, 38 ) );
    qim->setColor( i++, qRgb( 222, 255, 34 ) );
    qim->setColor( i++, qRgb( 226, 255, 30 ) );
    qim->setColor( i++, qRgb( 230, 255, 26 ) );
    qim->setColor( i++, qRgb( 234, 255, 22 ) );
    qim->setColor( i++, qRgb( 238, 255, 18 ) );
    qim->setColor( i++, qRgb( 242, 255, 14 ) );
    qim->setColor( i++, qRgb( 246, 255, 9 ) );
    qim->setColor( i++, qRgb( 251, 255, 4 ) );
    qim->setColor( i++, qRgb( 254, 255, 1 ) );
    qim->setColor( i++, qRgb( 255, 255, 0 ) );
    qim->setColor( i++, qRgb( 255, 252, 0 ) );
    qim->setColor( i++, qRgb( 255, 248, 0 ) );
    qim->setColor( i++, qRgb( 255, 243, 0 ) );
    qim->setColor( i++, qRgb( 255, 239, 0 ) );
    qim->setColor( i++, qRgb( 255, 235, 0 ) );
    qim->setColor( i++, qRgb( 255, 231, 0 ) );
    qim->setColor( i++, qRgb( 255, 227, 0 ) );
    qim->setColor( i++, qRgb( 255, 223, 0 ) );
    qim->setColor( i++, qRgb( 255, 219, 0 ) );
    qim->setColor( i++, qRgb( 255, 215, 0 ) );
    qim->setColor( i++, qRgb( 255, 211, 0 ) );
    qim->setColor( i++, qRgb( 255, 207, 0 ) );
    qim->setColor( i++, qRgb( 255, 203, 0 ) );
    qim->setColor( i++, qRgb( 255, 199, 0 ) );
    qim->setColor( i++, qRgb( 255, 195, 0 ) );
    qim->setColor( i++, qRgb( 255, 191, 0 ) );
    qim->setColor( i++, qRgb( 255, 187, 0 ) );
    qim->setColor( i++, qRgb( 255, 183, 0 ) );
    qim->setColor( i++, qRgb( 255, 179, 0 ) );
    qim->setColor( i++, qRgb( 255, 175, 0 ) );
    qim->setColor( i++, qRgb( 255, 171, 0 ) );
    qim->setColor( i++, qRgb( 255, 167, 0 ) );
    qim->setColor( i++, qRgb( 255, 163, 0 ) );
    qim->setColor( i++, qRgb( 255, 159, 0 ) );
    qim->setColor( i++, qRgb( 255, 155, 0 ) );
    qim->setColor( i++, qRgb( 255, 151, 0 ) );
    qim->setColor( i++, qRgb( 255, 147, 0 ) );
    qim->setColor( i++, qRgb( 255, 143, 0 ) );
    qim->setColor( i++, qRgb( 255, 139, 0 ) );
    qim->setColor( i++, qRgb( 255, 135, 0 ) );
    qim->setColor( i++, qRgb( 255, 131, 0 ) );
    qim->setColor( i++, qRgb( 255, 127, 0 ) );
    qim->setColor( i++, qRgb( 255, 123, 0 ) );
    qim->setColor( i++, qRgb( 255, 119, 0 ) );
    qim->setColor( i++, qRgb( 255, 115, 0 ) );
    qim->setColor( i++, qRgb( 255, 111, 0 ) );
    qim->setColor( i++, qRgb( 255, 107, 0 ) );
    qim->setColor( i++, qRgb( 255, 103, 0 ) );
    qim->setColor( i++, qRgb( 255, 99, 0 ) );
    qim->setColor( i++, qRgb( 255, 95, 0 ) );
    qim->setColor( i++, qRgb( 255, 91, 0 ) );
    qim->setColor( i++, qRgb( 255, 87, 0 ) );
    qim->setColor( i++, qRgb( 255, 83, 0 ) );
    qim->setColor( i++, qRgb( 255, 79, 0 ) );
    qim->setColor( i++, qRgb( 255, 75, 0 ) );
    qim->setColor( i++, qRgb( 255, 71, 0 ) );
    qim->setColor( i++, qRgb( 255, 67, 0 ) );
    qim->setColor( i++, qRgb( 255, 63, 0 ) );
    qim->setColor( i++, qRgb( 255, 59, 0 ) );
    qim->setColor( i++, qRgb( 255, 55, 0 ) );
    qim->setColor( i++, qRgb( 255, 51, 0 ) );
    qim->setColor( i++, qRgb( 255, 47, 0 ) );
    qim->setColor( i++, qRgb( 255, 43, 0 ) );
    qim->setColor( i++, qRgb( 255, 39, 0 ) );
    qim->setColor( i++, qRgb( 255, 35, 0 ) );
    qim->setColor( i++, qRgb( 255, 31, 0 ) );
    qim->setColor( i++, qRgb( 255, 27, 0 ) );
    qim->setColor( i++, qRgb( 255, 23, 0 ) );
    qim->setColor( i++, qRgb( 255, 19, 0 ) );
    qim->setColor( i++, qRgb( 255, 15, 0 ) );
    qim->setColor( i++, qRgb( 255, 11, 0 ) );
    qim->setColor( i++, qRgb( 255, 5, 0 ) );
    qim->setColor( i++, qRgb( 255, 1, 0 ) );
    qim->setColor( i++, qRgb( 255, 0, 0 ) );
    qim->setColor( i++, qRgb( 253, 0, 0 ) );
    qim->setColor( i++, qRgb( 249, 0, 0 ) );
    qim->setColor( i++, qRgb( 244, 0, 0 ) );
    qim->setColor( i++, qRgb( 240, 0, 0 ) );
    qim->setColor( i++, qRgb( 236, 0, 0 ) );
    qim->setColor( i++, qRgb( 232, 0, 0 ) );
    qim->setColor( i++, qRgb( 228, 0, 0 ) );
    qim->setColor( i++, qRgb( 224, 0, 0 ) );
    qim->setColor( i++, qRgb( 220, 0, 0 ) );
    qim->setColor( i++, qRgb( 216, 0, 0 ) );
    qim->setColor( i++, qRgb( 212, 0, 0 ) );
    qim->setColor( i++, qRgb( 208, 0, 0 ) );
    qim->setColor( i++, qRgb( 204, 0, 0 ) );
    qim->setColor( i++, qRgb( 200, 0, 0 ) );
    qim->setColor( i++, qRgb( 196, 0, 0 ) );
    qim->setColor( i++, qRgb( 192, 0, 0 ) );
    qim->setColor( i++, qRgb( 188, 0, 0 ) );
    qim->setColor( i++, qRgb( 184, 0, 0 ) );
    qim->setColor( i++, qRgb( 180, 0, 0 ) );
    qim->setColor( i++, qRgb( 176, 0, 0 ) );
    qim->setColor( i++, qRgb( 172, 0, 0 ) );
    qim->setColor( i++, qRgb( 168, 0, 0 ) );
    qim->setColor( i++, qRgb( 164, 0, 0 ) );
    qim->setColor( i++, qRgb( 160, 0, 0 ) );
    qim->setColor( i++, qRgb( 156, 0, 0 ) );
    qim->setColor( i++, qRgb( 152, 0, 0 ) );
    qim->setColor( i++, qRgb( 148, 0, 0 ) );
    qim->setColor( i++, qRgb( 144, 0, 0 ) );
    qim->setColor( i++, qRgb( 140, 0, 0 ) );
    qim->setColor( i++, qRgb( 136, 0, 0 ) );
    qim->setColor( i++, qRgb( 132, 0, 0 ) );
    qim->setColor( i, qRgb( 128, 0, 0 ) );

    qim->setColor( i + 1, qRgb( 0, 0, 0 ) );
    qim->setColor( i + 2, qRgb( 0, 255, 0 ) );
    return i;
#endif
#if 0
   //The 64-bit jet table on which the above is based
   qim->setColor(i++, qRgb(  0,   0, 144 ));
   qim->setColor(i++, qRgb(  0,   0, 160 ));
   qim->setColor(i++, qRgb(  0,   0, 176 ));
   qim->setColor(i++, qRgb(  0,   0, 192 ));
   qim->setColor(i++, qRgb(  0,   0, 208 ));
   qim->setColor(i++, qRgb(  0,   0, 224 ));
   qim->setColor(i++, qRgb(  0,   0, 240 ));
   qim->setColor(i++, qRgb(  0,   0, 255 ));
   qim->setColor(i++, qRgb(  0,  16  , 255 ));
   qim->setColor(i++, qRgb(  0,  32  , 255 ));
   qim->setColor(i++, qRgb(  0,  48  , 255 ));
   qim->setColor(i++, qRgb(  0,  64  , 255 ));
   qim->setColor(i++, qRgb(  0,  80, 255 ));
   qim->setColor(i++, qRgb(  0,  96  , 255 ));
   qim->setColor(i++, qRgb(  0, 112  , 255 ));
   qim->setColor(i++, qRgb(  0, 128  , 255 ));
   qim->setColor(i++, qRgb(  0, 144  , 255 ));
   qim->setColor(i++, qRgb(  0, 160, 255 ));
   qim->setColor(i++, qRgb(  0, 176  , 255 ));
   qim->setColor(i++, qRgb(  0, 192  , 255 ));
   qim->setColor(i++, qRgb( 0 , 208  , 255 ));
   qim->setColor(i++, qRgb(  0, 224  , 255 ));
   qim->setColor(i++, qRgb(  0, 240, 255 ));
   qim->setColor(i++, qRgb(  0, 255  , 255 ));
   qim->setColor(i++, qRgb( 16  , 255  , 240 ));
   qim->setColor(i++, qRgb( 32  , 255  , 224 ));
   qim->setColor(i++, qRgb( 48  , 255  , 208 ));
   qim->setColor(i++, qRgb( 64  , 255  , 192 ));
   qim->setColor(i++, qRgb( 80, 255  , 176 ));
   qim->setColor(i++, qRgb( 96  , 255  , 160 ));
   qim->setColor(i++, qRgb(112  , 255  , 144 ));
   qim->setColor(i++, qRgb(128  , 255  , 128 ));
   qim->setColor(i++, qRgb(144  , 255  , 112 ));
   qim->setColor(i++, qRgb(160, 255  ,  96 ));
   qim->setColor(i++, qRgb(176  , 255  ,  80 ));
   qim->setColor(i++, qRgb(192  , 255  ,  64 ));
   qim->setColor(i++, qRgb(208  , 255  ,  48 ));
   qim->setColor(i++, qRgb(224  , 255  ,  32 ));
   qim->setColor(i++, qRgb(240, 255  ,  16 ));
   qim->setColor(i++, qRgb(255  , 255  ,   0 ));
   qim->setColor(i++, qRgb(255  , 240,   0 ));
   qim->setColor(i++, qRgb(255  , 224  ,   0 ));
   qim->setColor(i++, qRgb(255  , 208  ,   0 ));
   qim->setColor(i++, qRgb(255  , 192  ,   0 ));
   qim->setColor(i++, qRgb(255  , 176  ,   0 ));
   qim->setColor(i++, qRgb(255  , 160,   0 ));
   qim->setColor(i++, qRgb(255  , 144  ,   0 ));
   qim->setColor(i++, qRgb(255  , 128  ,   0 ));
   qim->setColor(i++, qRgb(255  , 112  ,   0 ));
   qim->setColor(i++, qRgb(255  ,  96  ,   0 ));
   qim->setColor(i++, qRgb(255  ,  80,   0 ));
   qim->setColor(i++, qRgb(255  ,  64  ,   0 ));
   qim->setColor(i++, qRgb(255  ,  48  ,   0 ));
   qim->setColor(i++, qRgb(255  ,  32  ,   0 ));
   qim->setColor(i++, qRgb(255  ,  16  ,   0 ));
   qim->setColor(i++, qRgb(255  ,   0,   0 ));
   qim->setColor(i++, qRgb(240,   0,   0 ));
   qim->setColor(i++, qRgb(224  ,   0,   0 ));
   qim->setColor(i++, qRgb(208  ,   0,   0 ));
   qim->setColor(i++, qRgb(192  ,   0,   0 ));
   qim->setColor(i++, qRgb(176  ,   0,   0 ));
   qim->setColor(i++, qRgb(160,   0,   0 ));
   qim->setColor(i++, qRgb(144  ,   0,   0 ));
   qim->setColor(i, qRgb(128  ,   0,   0 ));
   
   qim->setColor(i+1,   qRgb(    0,      0,  0 ));
   qim->setColor(i+2,   qRgb(    255,      0,  0 ));
   
   return i;
#endif
}

/// Hot: load the Matlab Hot colormap
inline int load_colorbar_hot( QImage *qim )
{
    int i = 0;

    qim->setColor( i++, qRgb( 11, 0, 0 ) );
    qim->setColor( i++, qRgb( 21, 0, 0 ) );
    qim->setColor( i++, qRgb( 32, 0, 0 ) );
    qim->setColor( i++, qRgb( 43, 0, 0 ) );
    qim->setColor( i++, qRgb( 53, 0, 0 ) );
    qim->setColor( i++, qRgb( 64, 0, 0 ) );
    qim->setColor( i++, qRgb( 75, 0, 0 ) );
    qim->setColor( i++, qRgb( 85, 0, 0 ) );
    qim->setColor( i++, qRgb( 96, 0, 0 ) );
    qim->setColor( i++, qRgb( 107, 0, 0 ) );
    qim->setColor( i++, qRgb( 117, 0, 0 ) );
    qim->setColor( i++, qRgb( 128, 0, 0 ) );
    qim->setColor( i++, qRgb( 139, 0, 0 ) );
    qim->setColor( i++, qRgb( 149, 0, 0 ) );
    qim->setColor( i++, qRgb( 160, 0, 0 ) );
    qim->setColor( i++, qRgb( 171, 0, 0 ) );
    qim->setColor( i++, qRgb( 181, 0, 0 ) );
    qim->setColor( i++, qRgb( 192, 0, 0 ) );
    qim->setColor( i++, qRgb( 203, 0, 0 ) );
    qim->setColor( i++, qRgb( 213, 0, 0 ) );
    qim->setColor( i++, qRgb( 224, 0, 0 ) );
    qim->setColor( i++, qRgb( 235, 0, 0 ) );
    qim->setColor( i++, qRgb( 245, 0, 0 ) );
    qim->setColor( i++, qRgb( 255, 0, 0 ) );
    qim->setColor( i++, qRgb( 255, 11, 0 ) );
    qim->setColor( i++, qRgb( 255, 21, 0 ) );
    qim->setColor( i++, qRgb( 255, 32, 0 ) );
    qim->setColor( i++, qRgb( 255, 43, 0 ) );
    qim->setColor( i++, qRgb( 255, 53, 0 ) );
    qim->setColor( i++, qRgb( 255, 64, 0 ) );
    qim->setColor( i++, qRgb( 255, 75, 0 ) );
    qim->setColor( i++, qRgb( 255, 85, 0 ) );
    qim->setColor( i++, qRgb( 255, 96, 0 ) );
    qim->setColor( i++, qRgb( 255, 107, 0 ) );
    qim->setColor( i++, qRgb( 255, 117, 0 ) );
    qim->setColor( i++, qRgb( 255, 128, 0 ) );
    qim->setColor( i++, qRgb( 255, 139, 0 ) );
    qim->setColor( i++, qRgb( 255, 149, 0 ) );
    qim->setColor( i++, qRgb( 255, 160, 0 ) );
    qim->setColor( i++, qRgb( 255, 171, 0 ) );
    qim->setColor( i++, qRgb( 255, 181, 0 ) );
    qim->setColor( i++, qRgb( 255, 192, 0 ) );
    qim->setColor( i++, qRgb( 255, 202, 0 ) );
    qim->setColor( i++, qRgb( 255, 213, 0 ) );
    qim->setColor( i++, qRgb( 255, 224, 0 ) );
    qim->setColor( i++, qRgb( 255, 235, 0 ) );
    qim->setColor( i++, qRgb( 255, 245, 0 ) );
    qim->setColor( i++, qRgb( 255, 255, 0 ) );
    qim->setColor( i++, qRgb( 255, 255, 16 ) );
    qim->setColor( i++, qRgb( 255, 255, 32 ) );
    qim->setColor( i++, qRgb( 255, 255, 48 ) );
    qim->setColor( i++, qRgb( 255, 255, 64 ) );
    qim->setColor( i++, qRgb( 255, 255, 80 ) );
    qim->setColor( i++, qRgb( 255, 255, 96 ) );
    qim->setColor( i++, qRgb( 255, 255, 112 ) );
    qim->setColor( i++, qRgb( 255, 255, 128 ) );
    qim->setColor( i++, qRgb( 255, 255, 144 ) );
    qim->setColor( i++, qRgb( 255, 255, 160 ) );
    qim->setColor( i++, qRgb( 255, 255, 176 ) );
    qim->setColor( i++, qRgb( 255, 255, 192 ) );
    qim->setColor( i++, qRgb( 255, 255, 208 ) );
    qim->setColor( i++, qRgb( 255, 255, 224 ) );
    qim->setColor( i++, qRgb( 255, 255, 240 ) );
    qim->setColor( i, qRgb( 255, 255, 255 ) );

    qim->setColor( i + 1, qRgb( 0, 0, 0 ) );
    qim->setColor( i + 2, qRgb( 0, 0, 255 ) );

    return i;
}

/// Bone: load the Matlab Bone colormap
inline int load_colorbar_bone( QImage *qim )
{
    int i = 0;

#if 1
    // The matlab bone colortable c-spline interpolated onto 256 colors
    qim->setColor( i++, qRgb( 0, 0, 1 ) );
    qim->setColor( i++, qRgb( 1, 1, 2 ) );
    qim->setColor( i++, qRgb( 2, 2, 3 ) );
    qim->setColor( i++, qRgb( 3, 3, 5 ) );
    qim->setColor( i++, qRgb( 4, 4, 6 ) );
    qim->setColor( i++, qRgb( 5, 5, 7 ) );
    qim->setColor( i++, qRgb( 5, 5, 8 ) );
    qim->setColor( i++, qRgb( 6, 6, 10 ) );
    qim->setColor( i++, qRgb( 7, 7, 11 ) );
    qim->setColor( i++, qRgb( 8, 8, 12 ) );
    qim->setColor( i++, qRgb( 9, 9, 13 ) );
    qim->setColor( i++, qRgb( 10, 10, 15 ) );
    qim->setColor( i++, qRgb( 11, 11, 16 ) );
    qim->setColor( i++, qRgb( 12, 12, 17 ) );
    qim->setColor( i++, qRgb( 12, 12, 18 ) );
    qim->setColor( i++, qRgb( 13, 13, 20 ) );
    qim->setColor( i++, qRgb( 14, 14, 21 ) );
    qim->setColor( i++, qRgb( 15, 15, 22 ) );
    qim->setColor( i++, qRgb( 16, 16, 23 ) );
    qim->setColor( i++, qRgb( 17, 17, 25 ) );
    qim->setColor( i++, qRgb( 18, 18, 26 ) );
    qim->setColor( i++, qRgb( 19, 19, 27 ) );
    qim->setColor( i++, qRgb( 19, 19, 28 ) );
    qim->setColor( i++, qRgb( 20, 20, 30 ) );
    qim->setColor( i++, qRgb( 21, 21, 31 ) );
    qim->setColor( i++, qRgb( 22, 22, 32 ) );
    qim->setColor( i++, qRgb( 23, 23, 33 ) );
    qim->setColor( i++, qRgb( 24, 24, 34 ) );
    qim->setColor( i++, qRgb( 25, 25, 35 ) );
    qim->setColor( i++, qRgb( 26, 26, 36 ) );
    qim->setColor( i++, qRgb( 26, 26, 37 ) );
    qim->setColor( i++, qRgb( 27, 27, 39 ) );
    qim->setColor( i++, qRgb( 28, 28, 40 ) );
    qim->setColor( i++, qRgb( 29, 29, 41 ) );
    qim->setColor( i++, qRgb( 30, 30, 42 ) );
    qim->setColor( i++, qRgb( 31, 31, 44 ) );
    qim->setColor( i++, qRgb( 32, 32, 45 ) );
    qim->setColor( i++, qRgb( 33, 33, 46 ) );
    qim->setColor( i++, qRgb( 33, 33, 47 ) );
    qim->setColor( i++, qRgb( 34, 34, 49 ) );
    qim->setColor( i++, qRgb( 35, 35, 50 ) );
    qim->setColor( i++, qRgb( 36, 36, 51 ) );
    qim->setColor( i++, qRgb( 37, 37, 52 ) );
    qim->setColor( i++, qRgb( 38, 38, 54 ) );
    qim->setColor( i++, qRgb( 39, 39, 55 ) );
    qim->setColor( i++, qRgb( 40, 40, 56 ) );
    qim->setColor( i++, qRgb( 41, 41, 57 ) );
    qim->setColor( i++, qRgb( 42, 42, 59 ) );
    qim->setColor( i++, qRgb( 43, 43, 60 ) );
    qim->setColor( i++, qRgb( 44, 44, 61 ) );
    qim->setColor( i++, qRgb( 44, 44, 62 ) );
    qim->setColor( i++, qRgb( 45, 45, 63 ) );
    qim->setColor( i++, qRgb( 46, 46, 65 ) );
    qim->setColor( i++, qRgb( 47, 47, 66 ) );
    qim->setColor( i++, qRgb( 48, 48, 67 ) );
    qim->setColor( i++, qRgb( 49, 49, 69 ) );
    qim->setColor( i++, qRgb( 50, 50, 70 ) );
    qim->setColor( i++, qRgb( 51, 51, 71 ) );
    qim->setColor( i++, qRgb( 51, 51, 72 ) );
    qim->setColor( i++, qRgb( 52, 52, 73 ) );
    qim->setColor( i++, qRgb( 53, 53, 74 ) );
    qim->setColor( i++, qRgb( 54, 54, 75 ) );
    qim->setColor( i++, qRgb( 55, 55, 76 ) );
    qim->setColor( i++, qRgb( 56, 56, 77 ) );
    qim->setColor( i++, qRgb( 57, 57, 79 ) );
    qim->setColor( i++, qRgb( 58, 58, 80 ) );
    qim->setColor( i++, qRgb( 58, 58, 81 ) );
    qim->setColor( i++, qRgb( 59, 59, 82 ) );
    qim->setColor( i++, qRgb( 60, 60, 84 ) );
    qim->setColor( i++, qRgb( 61, 61, 85 ) );
    qim->setColor( i++, qRgb( 62, 62, 86 ) );
    qim->setColor( i++, qRgb( 63, 63, 87 ) );
    qim->setColor( i++, qRgb( 64, 64, 89 ) );
    qim->setColor( i++, qRgb( 65, 65, 90 ) );
    qim->setColor( i++, qRgb( 65, 65, 91 ) );
    qim->setColor( i++, qRgb( 66, 66, 92 ) );
    qim->setColor( i++, qRgb( 67, 67, 94 ) );
    qim->setColor( i++, qRgb( 68, 68, 95 ) );
    qim->setColor( i++, qRgb( 69, 69, 96 ) );
    qim->setColor( i++, qRgb( 70, 70, 97 ) );
    qim->setColor( i++, qRgb( 71, 71, 99 ) );
    qim->setColor( i++, qRgb( 72, 72, 100 ) );
    qim->setColor( i++, qRgb( 72, 72, 101 ) );
    qim->setColor( i++, qRgb( 73, 73, 102 ) );
    qim->setColor( i++, qRgb( 74, 74, 104 ) );
    qim->setColor( i++, qRgb( 75, 75, 105 ) );
    qim->setColor( i++, qRgb( 76, 76, 106 ) );
    qim->setColor( i++, qRgb( 77, 77, 107 ) );
    qim->setColor( i++, qRgb( 78, 78, 108 ) );
    qim->setColor( i++, qRgb( 79, 79, 109 ) );
    qim->setColor( i++, qRgb( 79, 79, 110 ) );
    qim->setColor( i++, qRgb( 80, 80, 111 ) );
    qim->setColor( i++, qRgb( 81, 81, 113 ) );
    qim->setColor( i++, qRgb( 82, 82, 114 ) );
    qim->setColor( i++, qRgb( 82, 83, 115 ) );
    qim->setColor( i++, qRgb( 84, 84, 116 ) );
    qim->setColor( i++, qRgb( 85, 85, 117 ) );
    qim->setColor( i++, qRgb( 86, 87, 117 ) );
    qim->setColor( i++, qRgb( 87, 88, 118 ) );
    qim->setColor( i++, qRgb( 88, 89, 119 ) );
    qim->setColor( i++, qRgb( 89, 91, 120 ) );
    qim->setColor( i++, qRgb( 89, 92, 121 ) );
    qim->setColor( i++, qRgb( 90, 93, 121 ) );
    qim->setColor( i++, qRgb( 91, 94, 122 ) );
    qim->setColor( i++, qRgb( 92, 95, 124 ) );
    qim->setColor( i++, qRgb( 93, 97, 125 ) );
    qim->setColor( i++, qRgb( 94, 98, 126 ) );
    qim->setColor( i++, qRgb( 95, 99, 127 ) );
    qim->setColor( i++, qRgb( 96, 100, 128 ) );
    qim->setColor( i++, qRgb( 96, 102, 128 ) );
    qim->setColor( i++, qRgb( 97, 103, 129 ) );
    qim->setColor( i++, qRgb( 98, 104, 130 ) );
    qim->setColor( i++, qRgb( 99, 105, 131 ) );
    qim->setColor( i++, qRgb( 100, 107, 132 ) );
    qim->setColor( i++, qRgb( 101, 108, 133 ) );
    qim->setColor( i++, qRgb( 102, 109, 134 ) );
    qim->setColor( i++, qRgb( 103, 110, 135 ) );
    qim->setColor( i++, qRgb( 103, 112, 135 ) );
    qim->setColor( i++, qRgb( 104, 113, 136 ) );
    qim->setColor( i++, qRgb( 105, 114, 137 ) );
    qim->setColor( i++, qRgb( 106, 115, 138 ) );
    qim->setColor( i++, qRgb( 106, 117, 138 ) );
    qim->setColor( i++, qRgb( 107, 118, 139 ) );
    qim->setColor( i++, qRgb( 109, 118, 141 ) );
    qim->setColor( i++, qRgb( 110, 119, 142 ) );
    qim->setColor( i++, qRgb( 110, 121, 142 ) );
    qim->setColor( i++, qRgb( 111, 122, 143 ) );
    qim->setColor( i++, qRgb( 112, 123, 144 ) );
    qim->setColor( i++, qRgb( 113, 124, 145 ) );
    qim->setColor( i++, qRgb( 113, 126, 145 ) );
    qim->setColor( i++, qRgb( 114, 127, 146 ) );
    qim->setColor( i++, qRgb( 116, 128, 148 ) );
    qim->setColor( i++, qRgb( 117, 129, 149 ) );
    qim->setColor( i++, qRgb( 117, 131, 149 ) );
    qim->setColor( i++, qRgb( 118, 132, 150 ) );
    qim->setColor( i++, qRgb( 119, 133, 151 ) );
    qim->setColor( i++, qRgb( 120, 134, 152 ) );
    qim->setColor( i++, qRgb( 120, 136, 152 ) );
    qim->setColor( i++, qRgb( 121, 137, 153 ) );
    qim->setColor( i++, qRgb( 122, 138, 154 ) );
    qim->setColor( i++, qRgb( 123, 139, 155 ) );
    qim->setColor( i++, qRgb( 124, 141, 156 ) );
    qim->setColor( i++, qRgb( 126, 142, 157 ) );
    qim->setColor( i++, qRgb( 127, 143, 158 ) );
    qim->setColor( i++, qRgb( 128, 144, 159 ) );
    qim->setColor( i++, qRgb( 128, 146, 159 ) );
    qim->setColor( i++, qRgb( 129, 147, 160 ) );
    qim->setColor( i++, qRgb( 130, 148, 161 ) );
    qim->setColor( i++, qRgb( 131, 149, 162 ) );
    qim->setColor( i++, qRgb( 131, 151, 163 ) );
    qim->setColor( i++, qRgb( 132, 152, 164 ) );
    qim->setColor( i++, qRgb( 133, 153, 165 ) );
    qim->setColor( i++, qRgb( 134, 154, 166 ) );
    qim->setColor( i++, qRgb( 135, 155, 166 ) );
    qim->setColor( i++, qRgb( 136, 156, 167 ) );
    qim->setColor( i++, qRgb( 137, 157, 168 ) );
    qim->setColor( i++, qRgb( 137, 158, 169 ) );
    qim->setColor( i++, qRgb( 138, 159, 170 ) );
    qim->setColor( i++, qRgb( 139, 161, 171 ) );
    qim->setColor( i++, qRgb( 140, 162, 173 ) );
    qim->setColor( i++, qRgb( 141, 163, 173 ) );
    qim->setColor( i++, qRgb( 142, 164, 174 ) );
    qim->setColor( i++, qRgb( 143, 166, 175 ) );
    qim->setColor( i++, qRgb( 144, 167, 176 ) );
    qim->setColor( i++, qRgb( 144, 168, 176 ) );
    qim->setColor( i++, qRgb( 145, 169, 177 ) );
    qim->setColor( i++, qRgb( 146, 171, 178 ) );
    qim->setColor( i++, qRgb( 147, 172, 179 ) );
    qim->setColor( i++, qRgb( 148, 173, 180 ) );
    qim->setColor( i++, qRgb( 149, 174, 181 ) );
    qim->setColor( i++, qRgb( 150, 176, 182 ) );
    qim->setColor( i++, qRgb( 151, 177, 183 ) );
    qim->setColor( i++, qRgb( 151, 178, 183 ) );
    qim->setColor( i++, qRgb( 152, 179, 184 ) );
    qim->setColor( i++, qRgb( 153, 181, 185 ) );
    qim->setColor( i++, qRgb( 154, 182, 186 ) );
    qim->setColor( i++, qRgb( 155, 183, 187 ) );
    qim->setColor( i++, qRgb( 156, 184, 188 ) );
    qim->setColor( i++, qRgb( 157, 186, 189 ) );
    qim->setColor( i++, qRgb( 158, 187, 190 ) );
    qim->setColor( i++, qRgb( 158, 188, 190 ) );
    qim->setColor( i++, qRgb( 159, 189, 191 ) );
    qim->setColor( i++, qRgb( 160, 190, 192 ) );
    qim->setColor( i++, qRgb( 161, 191, 193 ) );
    qim->setColor( i++, qRgb( 162, 192, 194 ) );
    qim->setColor( i++, qRgb( 163, 193, 195 ) );
    qim->setColor( i++, qRgb( 164, 195, 196 ) );
    qim->setColor( i++, qRgb( 165, 196, 197 ) );
    qim->setColor( i++, qRgb( 165, 197, 197 ) );
    qim->setColor( i++, qRgb( 166, 198, 198 ) );
    qim->setColor( i++, qRgb( 168, 199, 199 ) );
    qim->setColor( i++, qRgb( 169, 200, 200 ) );
    qim->setColor( i++, qRgb( 171, 201, 201 ) );
    qim->setColor( i++, qRgb( 172, 202, 202 ) );
    qim->setColor( i++, qRgb( 174, 203, 203 ) );
    qim->setColor( i++, qRgb( 176, 204, 204 ) );
    qim->setColor( i++, qRgb( 177, 204, 204 ) );
    qim->setColor( i++, qRgb( 178, 205, 205 ) );
    qim->setColor( i++, qRgb( 180, 206, 206 ) );
    qim->setColor( i++, qRgb( 181, 207, 207 ) );
    qim->setColor( i++, qRgb( 182, 208, 208 ) );
    qim->setColor( i++, qRgb( 183, 209, 209 ) );
    qim->setColor( i++, qRgb( 185, 210, 210 ) );
    qim->setColor( i++, qRgb( 186, 211, 211 ) );
    qim->setColor( i++, qRgb( 188, 212, 212 ) );
    qim->setColor( i++, qRgb( 189, 213, 213 ) );
    qim->setColor( i++, qRgb( 191, 214, 214 ) );
    qim->setColor( i++, qRgb( 192, 215, 215 ) );
    qim->setColor( i++, qRgb( 193, 215, 215 ) );
    qim->setColor( i++, qRgb( 194, 216, 216 ) );
    qim->setColor( i++, qRgb( 196, 217, 217 ) );
    qim->setColor( i++, qRgb( 197, 218, 218 ) );
    qim->setColor( i++, qRgb( 199, 219, 219 ) );
    qim->setColor( i++, qRgb( 200, 220, 220 ) );
    qim->setColor( i++, qRgb( 201, 221, 221 ) );
    qim->setColor( i++, qRgb( 203, 222, 222 ) );
    qim->setColor( i++, qRgb( 204, 222, 222 ) );
    qim->setColor( i++, qRgb( 205, 223, 223 ) );
    qim->setColor( i++, qRgb( 207, 224, 224 ) );
    qim->setColor( i++, qRgb( 208, 225, 225 ) );
    qim->setColor( i++, qRgb( 210, 226, 226 ) );
    qim->setColor( i++, qRgb( 211, 227, 227 ) );
    qim->setColor( i++, qRgb( 212, 228, 228 ) );
    qim->setColor( i++, qRgb( 214, 229, 229 ) );
    qim->setColor( i++, qRgb( 215, 229, 229 ) );
    qim->setColor( i++, qRgb( 216, 230, 230 ) );
    qim->setColor( i++, qRgb( 218, 231, 231 ) );
    qim->setColor( i++, qRgb( 219, 232, 232 ) );
    qim->setColor( i++, qRgb( 221, 233, 233 ) );
    qim->setColor( i++, qRgb( 222, 234, 234 ) );
    qim->setColor( i++, qRgb( 223, 235, 235 ) );
    qim->setColor( i++, qRgb( 225, 236, 236 ) );
    qim->setColor( i++, qRgb( 226, 236, 236 ) );
    qim->setColor( i++, qRgb( 227, 237, 237 ) );
    qim->setColor( i++, qRgb( 229, 238, 238 ) );
    qim->setColor( i++, qRgb( 230, 239, 239 ) );
    qim->setColor( i++, qRgb( 232, 240, 240 ) );
    qim->setColor( i++, qRgb( 233, 241, 241 ) );
    qim->setColor( i++, qRgb( 234, 242, 242 ) );
    qim->setColor( i++, qRgb( 236, 243, 243 ) );
    qim->setColor( i++, qRgb( 237, 243, 243 ) );
    qim->setColor( i++, qRgb( 238, 244, 244 ) );
    qim->setColor( i++, qRgb( 240, 245, 245 ) );
    qim->setColor( i++, qRgb( 241, 246, 246 ) );
    qim->setColor( i++, qRgb( 243, 247, 247 ) );
    qim->setColor( i++, qRgb( 244, 248, 248 ) );
    qim->setColor( i++, qRgb( 245, 249, 249 ) );
    qim->setColor( i++, qRgb( 247, 250, 250 ) );
    qim->setColor( i++, qRgb( 248, 250, 250 ) );
    qim->setColor( i++, qRgb( 249, 251, 251 ) );
    qim->setColor( i++, qRgb( 250, 252, 252 ) );
    qim->setColor( i++, qRgb( 252, 253, 253 ) );
    qim->setColor( i++, qRgb( 253, 254, 254 ) );
    qim->setColor( i, qRgb( 255, 255, 255 ) );

    qim->setColor( i + 1, qRgb( 0, 0, 0 ) );
    qim->setColor( i + 2, qRgb( 255, 0, 0 ) );

#else
    // The matlab bone colortable, original 64 color version
    qim->setColor( i++, qRgb( 0, 0, 1 ) );
    qim->setColor( i++, qRgb( 4, 4, 6 ) );
    qim->setColor( i++, qRgb( 7, 7, 11 ) );
    qim->setColor( i++, qRgb( 11, 11, 16 ) );
    qim->setColor( i++, qRgb( 14, 14, 21 ) );
    qim->setColor( i++, qRgb( 18, 18, 26 ) );
    qim->setColor( i++, qRgb( 21, 21, 31 ) );
    qim->setColor( i++, qRgb( 25, 25, 35 ) );
    qim->setColor( i++, qRgb( 28, 28, 40 ) );
    qim->setColor( i++, qRgb( 32, 32, 45 ) );
    qim->setColor( i++, qRgb( 35, 35, 50 ) );
    qim->setColor( i++, qRgb( 39, 39, 55 ) );
    qim->setColor( i++, qRgb( 43, 43, 60 ) );
    qim->setColor( i++, qRgb( 46, 46, 65 ) );
    qim->setColor( i++, qRgb( 50, 50, 70 ) );
    qim->setColor( i++, qRgb( 53, 53, 74 ) );
    qim->setColor( i++, qRgb( 57, 57, 79 ) );
    qim->setColor( i++, qRgb( 60, 60, 84 ) );
    qim->setColor( i++, qRgb( 64, 64, 89 ) );
    qim->setColor( i++, qRgb( 67, 67, 94 ) );
    qim->setColor( i++, qRgb( 71, 71, 99 ) );
    qim->setColor( i++, qRgb( 74, 74, 104 ) );
    qim->setColor( i++, qRgb( 78, 78, 108 ) );
    qim->setColor( i++, qRgb( 81, 81, 113 ) );
    qim->setColor( i++, qRgb( 85, 86, 117 ) );
    qim->setColor( i++, qRgb( 89, 91, 120 ) );
    qim->setColor( i++, qRgb( 92, 96, 124 ) );
    qim->setColor( i++, qRgb( 96, 101, 128 ) );
    qim->setColor( i++, qRgb( 99, 106, 131 ) );
    qim->setColor( i++, qRgb( 103, 111, 135 ) );
    qim->setColor( i++, qRgb( 106, 116, 138 ) );
    qim->setColor( i++, qRgb( 110, 120, 142 ) );
    qim->setColor( i++, qRgb( 113, 125, 145 ) );
    qim->setColor( i++, qRgb( 117, 130, 149 ) );
    qim->setColor( i++, qRgb( 120, 135, 152 ) );
    qim->setColor( i++, qRgb( 124, 140, 156 ) );
    qim->setColor( i++, qRgb( 128, 145, 159 ) );
    qim->setColor( i++, qRgb( 131, 150, 163 ) );
    qim->setColor( i++, qRgb( 135, 155, 166 ) );
    qim->setColor( i++, qRgb( 138, 159, 170 ) );
    qim->setColor( i++, qRgb( 142, 164, 174 ) );
    qim->setColor( i++, qRgb( 145, 169, 177 ) );
    qim->setColor( i++, qRgb( 149, 174, 181 ) );
    qim->setColor( i++, qRgb( 152, 179, 184 ) );
    qim->setColor( i++, qRgb( 156, 184, 188 ) );
    qim->setColor( i++, qRgb( 159, 189, 191 ) );
    qim->setColor( i++, qRgb( 163, 193, 195 ) );
    qim->setColor( i++, qRgb( 166, 198, 198 ) );
    qim->setColor( i++, qRgb( 172, 202, 202 ) );
    qim->setColor( i++, qRgb( 178, 205, 205 ) );
    qim->setColor( i++, qRgb( 183, 209, 209 ) );
    qim->setColor( i++, qRgb( 189, 213, 213 ) );
    qim->setColor( i++, qRgb( 194, 216, 216 ) );
    qim->setColor( i++, qRgb( 200, 220, 220 ) );
    qim->setColor( i++, qRgb( 205, 223, 223 ) );
    qim->setColor( i++, qRgb( 211, 227, 227 ) );
    qim->setColor( i++, qRgb( 216, 230, 230 ) );
    qim->setColor( i++, qRgb( 222, 234, 234 ) );
    qim->setColor( i++, qRgb( 227, 237, 237 ) );
    qim->setColor( i++, qRgb( 233, 241, 241 ) );
    qim->setColor( i++, qRgb( 238, 244, 244 ) );
    qim->setColor( i++, qRgb( 244, 248, 248 ) );
    qim->setColor( i++, qRgb( 249, 251, 251 ) );
    qim->setColor( i, qRgb( 255, 255, 255 ) );
#endif
    return i;
}

#endif // colorMaps_hpp
