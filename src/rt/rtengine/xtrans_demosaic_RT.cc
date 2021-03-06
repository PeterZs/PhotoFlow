////////////////////////////////////////////////////////////////
//
//			AMaZE demosaic algorithm
// (Aliasing Minimization and Zipper Elimination)
//
//	copyright (c) 2008-2010  Emil Martinec <ejmartin@uchicago.edu>
//
// incorporating ideas of Luis Sanz Rodrigues and Paul Lee
//
// code dated: May 27, 2010
//
//	amaze_interpolate_RT.cc is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////

#include <string.h>

//#include "rtengine.h"
#include "rawimagesource.hh"
#include "rt_math.h"
//#include "../rtgui/multilangmgr.h"
//#include "procparams.h"
#include "sleef.c"
#include "opthelper.h"

#undef CLIP
#define CLIP(x) x

#define FORC(cnt) for (c=0; c < cnt; c++)
#define FORC3 FORC(3)

#define fcol(r,c) ((int)(rawData[r].color(c)))
#define isgreen(row,col) (((int)(rawData[row].color(col)))&1)

//#define fcol(row,col) xtrans[(row)%6][(col)%6]
//#define isgreen(row,col) (xtrans[(row)%3][(col)%3]&1)

#undef _OPENMP
#undef __SSE2__

//namespace rtengine {
namespace rtengine {

SSEFUNCTION void RawImageSource::xtrans_demosaic_RT(int winx, int winy, int winw, int winh,
    int tilex, int tiley, int tilew, int tileh)
{

#define HCLIP(x) x //is this still necessary???
  //min(clip_pt,x)

  constexpr int ts = 114;      /* Tile Size */
  constexpr int tsh = ts / 2;  /* half of Tile Size */

  bool verbose = false;
  //if( winx<100 && winy > 3000 ) verbose = true;

  double progress = 0.0;
  const bool plistenerActive = plistener;

  //char xtrans[6][6];
  //ri->getXtransMatrix(xtrans);

  constexpr short  orth[12] = { 1, 0, 0, 1, -1, 0, 0, -1, 1, 0, 0, 1 },
      patt[2][16] = { { 0, 1, 0, -1, 2, 0, -1, 0, 1, 1, 1, -1, 0, 0, 0, 0 },
          { 0, 1, 0, -2, 1, 0, -2, 0, 1, 1, -2, -2, 1, -1, -1, 1 }
  },
  dir[4] = { 1, ts, ts + 1, ts - 1 };

  // sgrow/sgcol is the offset in the sensor matrix of the solitary
  // green pixels
  unsigned short sgrow, sgcol;

  const int height = winh, width = winw;

  int passes = 3;
  bool useCieLab = false;

  //printf("%d-pass X-Trans interpolation using %s conversion...\n", passes, useCieLab ? "lab" : "yuv");

  //xtransborder_interpolate(6);

#ifdef HAS_CIELAB
  float xyz_cam[3][3];
  {
    float rgb_cam[3][4];
    ri->getRgbCam(rgb_cam);
    int k;

    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        for (xyz_cam[i][j] = k = 0; k < 3; k++) {
          xyz_cam[i][j] += xyz_rgb[i][k] * rgb_cam[k][j] / d65_white[i];
        }
  }
#endif

  /* Map a green hexagon around each non-green pixel and vice versa:  */
  int allhex[2][3][3][8];
  {
    int gint, d, h, v, ng, row, col, c;
    for (row = 0; row < 3; row++)
      for (col = 0; col < 3; col++) {
        gint = isgreen(row, col);
        //printf("r=%d c=%d  col=%d gint=%d\n", row, col, (int)fcol(row,col), gint);

        for (ng = d = 0; d < 10; d += 2) {
          //printf("  r=%d c=%d  col=%d\n", row + orth[d] + 6, col + orth[d + 2] + 6, (int)fcol(row + orth[d] + 6,col + orth[d + 2] + 6));
          if (isgreen(row + orth[d] + 6, col + orth[d + 2] + 6)) {
            ng = 0;
          } else {
            ng++;
          }

          if (ng == 4) {
            // if there are four non-green pixels adjacent in cardinal
            // directions, this is the solitary green pixel
            sgrow = row;
            sgcol = col;
          }

          if (ng == gint + 1)
            FORC(8) {
            v = orth[d] * patt[gint][c * 2] + orth[d + 1] * patt[gint][c * 2 + 1];
            h = orth[d + 2] * patt[gint][c * 2] + orth[d + 3] * patt[gint][c * 2 + 1];
            //allhex[0][row][col][c ^ (gint * 2 & d)] = h + v * width;
            //allhex[1][row][col][c ^ (gint * 2 & d)] = h + v * ts;
            allhex[0][row][col][c ^ (gint * 2 & d)] = h + v * width;
            allhex[1][row][col][c ^ (gint * 2 & d)] = h + v * ts;
            if(verbose) {printf("r=%d c=%d v=%d h=%d allhex[%d]=%d\n",
                row, col, v, h, (int)(c ^ (gint * 2 & d)),
                allhex[0][row][col][c ^ (gint * 2 & d)]);}
          }
        }
      }
    /*
    for (row = 0; row < 3; row++) {
      for (col = 0; col < 3; col++) {
        for (ng = 0; ng < 8; ng++) {
          if( allhex[0][row][col][ng] > 1000 ) {
            printf("CFA pattern:\n");
            for (row = 0; row < 16; row++) {
              for (col = 0; col < 16; col++) {
                printf("%d ", (int)fcol(row,col));
              }
              printf("\n");
            }
          }
        }
      }
    }
    */
  }


  const int ndir = 4 << (passes > 1);
  //cielab (0, 0, 0, 0, 0, 0, 0, 0);
  struct s_minmaxgreen {
    float min;
    float max;
  };

  int RightShift[3];

  for(int row = 0; row < 3; row++) {
    // count number of green pixels in three cols
    int greencount = 0;

    for(int col = 0; col < 3; col++) {
      greencount += isgreen(row, col);
    }

    RightShift[row] = (greencount == 2);
  }

#ifdef _OPENMP
#pragma omp parallel
#endif
  {
    int progressCounter = 0;
    int c;
    float color[3][6];

    float *buffer = (float *) malloc ((ts * ts * (ndir * 4 + 3) + 128) * sizeof(float));
    float (*rgb)[ts][ts][3] = (float(*)[ts][ts][3]) buffer;
    float (*lab)[ts - 8][ts - 8] = (float (*)[ts - 8][ts - 8])(buffer + ts * ts * (ndir * 3));
    float (*drv)[ts - 10][ts - 10] = (float (*)[ts - 10][ts - 10])   (buffer + ts * ts * (ndir * 3 + 3));
    uint8_t (*homo)[ts][ts] = (uint8_t  (*)[ts][ts])   (lab); // we can reuse the lab-buffer because they are not used together
    s_minmaxgreen  (*greenminmaxtile)[tsh] = (s_minmaxgreen(*)[tsh]) (lab); // we can reuse the lab-buffer because they are not used together
    uint8_t (*homosum)[ts][ts] = (uint8_t (*)[ts][ts]) (drv); // we can reuse the drv-buffer because they are not used together
    uint8_t (*homosummax)[ts] = (uint8_t (*)[ts]) homo[ndir - 1]; // we can reuse the homo-buffer because they are not used together

#ifdef _OPENMP
#pragma omp for collapse(2) schedule(dynamic) nowait
#endif

    for (int top = 3; top < height - 19; top += ts - 16)
      for (int left = 3; left < width - 19; left += ts - 16) {
        int mrow = MIN (top + ts, height - 3);
        int mcol = MIN (left + ts, width - 3);

        int *thex = allhex[0][0][0];
        int thexid = 1;
        if(verbose) {printf("(%d)  thex[0][0]: %p  [", thexid, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thexid++;}
        /* Set greenmin and greenmax to the minimum and maximum allowed values: */
        for (int row = top; row < mrow; row++) {
          // find first non-green pixel
          int leftstart = left;

          for(; leftstart < mcol; leftstart++)
            if(!isgreen(row, leftstart)) {
              break;
            }

          int coloffset = (RightShift[row % 3] == 1 ? 3 : 1 + (fcol(row, leftstart + 1) & 1));

          if(coloffset == 3) {
            int *hex = allhex[0][row % 3][leftstart % 3];

            for (int col = leftstart; col < mcol; col += coloffset) {
              float minval = FLT_MAX;
              float maxval = 0.f;
              float *pix = &rawDataBuf[row][col];

              for(int c = 0; c < 6; c++) {
                float val = pix[hex[c]];

                minval = minval < val ? minval : val;
                maxval = maxval > val ? maxval : val;
              }

              greenminmaxtile[row - top][(col - left) >> 1].min = minval;
              greenminmaxtile[row - top][(col - left) >> 1].max = maxval;
            }
          } else {
            float minval = FLT_MAX;
            float maxval = 0.f;
            int col = leftstart;

            if(coloffset == 2) {
              minval = FLT_MAX;
              maxval = 0.f;
              float *pix = &rawDataBuf[row][col];
              int *hex = allhex[0][row % 3][col % 3];

              for(int c = 0; c < 6; c++) {
                float val = pix[hex[c]];

                minval = minval < val ? minval : val;
                maxval = maxval > val ? maxval : val;
              }

              greenminmaxtile[row - top][(col - left) >> 1].min = minval;
              greenminmaxtile[row - top][(col - left) >> 1].max = maxval;
              col += 2;
            }

            int *hex = allhex[0][row % 3][col % 3];
            //printf("row: %d  col: %d\n",(int)(row),(int)(col));
            //printf("hex[%d][%d]: %p  [", (int)(row%3),(int)(col%3), (void*)(hex));
            //for(int c=0; c<8; c++) printf("%d ",hex[c]);
            //printf("]\n");

            for (; col < mcol - 1; col += 3) {
              minval = FLT_MAX;
              maxval = 0.f;
              float *pix = &rawDataBuf[row][col];

              for(int c = 0; c < 6; c++) {
                float val = pix[hex[c]];

                minval = minval < val ? minval : val;
                maxval = maxval > val ? maxval : val;
              }

              greenminmaxtile[row - top][(col - left) >> 1].min = minval;
              greenminmaxtile[row - top][(col - left) >> 1].max = maxval;
              greenminmaxtile[row - top][(col + 1 - left) >> 1].min = minval;
              greenminmaxtile[row - top][(col + 1 - left) >> 1].max = maxval;
            }

            if(col < mcol) {
              minval = FLT_MAX;
              maxval = 0.f;
              float *pix = &rawDataBuf[row][col];

              for(int c = 0; c < 6; c++) {
                float val = pix[hex[c]];

                minval = minval < val ? minval : val;
                maxval = maxval > val ? maxval : val;
              }

              greenminmaxtile[row - top][(col - left) >> 1].min = minval;
              greenminmaxtile[row - top][(col - left) >> 1].max = maxval;
            }
          }
        }

        memset(rgb, 0, ts * ts * 3 * sizeof(float));

        if(verbose) {printf("(%d)  thex[0][0]: %p  [", thexid, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thexid++;}
        for (int row = top; row < mrow; row++)
          for (int col = left; col < mcol; col++) {
            rgb[0][row - top][col - left][fcol(row, col)] = rawData[row][col];
          }

        for(int c = 0; c < 3; c++) {
          memcpy (rgb[c + 1], rgb[0], sizeof * rgb);
        }

        if(verbose) {printf("(%d)  thex[0][0]: %p  [", thexid, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thexid++;}
        /* Interpolate green horizontally, vertically, and along both diagonals: */
        for (int row = top; row < mrow; row++) {
          // find first non-green pixel
          int leftstart = left;

          for(; leftstart < mcol; leftstart++)
            if(!isgreen(row, leftstart)) {
              break;
            }

          int coloffset = (RightShift[row % 3] == 1 ? 3 : 1 + (fcol(row, leftstart + 1) & 1));

          if(coloffset == 3) {
            int *hex = allhex[0][row % 3][leftstart % 3];

            for (int col = leftstart; col < mcol; col += coloffset) {
              float *pix = &rawDataBuf[row][col];
              float color[4];
              color[0] = 0.6796875f * (pix[hex[1]] + pix[hex[0]]) -
                  0.1796875f * (pix[2 * hex[1]] + pix[2 * hex[0]]);
              color[1] = 0.87109375f *  pix[hex[3]] + pix[hex[2]] * 0.12890625f +
                  0.359375f * (pix[0] - pix[-hex[2]]);

              for(int c = 0; c < 2; c++)
                color[2 + c] = 0.640625f * pix[hex[4 + c]] + 0.359375f * pix[-2 * hex[4 + c]] + 0.12890625f *
                (2.f * pix[0] - pix[3 * hex[4 + c]] - pix[-3 * hex[4 + c]]);

              for(int c = 0; c < 4; c++) {
                rgb[c][row - top][col - left][1] = LIM(color[c], greenminmaxtile[row - top][(col - left) >> 1].min, greenminmaxtile[row - top][(col - left) >> 1].max);
              }
            }
          } else {
            int *hexmod[2];
            hexmod[0] = allhex[0][row % 3][leftstart % 3];
            hexmod[1] = allhex[0][row % 3][(leftstart + coloffset) % 3];

            for (int col = leftstart, hexindex = 0; col < mcol; col += coloffset, coloffset ^= 3, hexindex ^= 1) {
              float *pix = &rawDataBuf[row][col];
              int *hex = hexmod[hexindex];
              float color[4];
              color[0] = 0.6796875f * (pix[hex[1]] + pix[hex[0]]) -
                  0.1796875f * (pix[2 * hex[1]] + pix[2 * hex[0]]);
              color[1] = 0.87109375f *  pix[hex[3]] + pix[hex[2]] * 0.12890625f +
                  0.359375f * (pix[0] - pix[-hex[2]]);

              for(int c = 0; c < 2; c++)
                color[2 + c] = 0.640625f * pix[hex[4 + c]] + 0.359375f * pix[-2 * hex[4 + c]] + 0.12890625f *
                (2.f * pix[0] - pix[3 * hex[4 + c]] - pix[-3 * hex[4 + c]]);

              for(int c = 0; c < 4; c++) {
                rgb[c ^ 1][row - top][col - left][1] = LIM(color[c], greenminmaxtile[row - top][(col - left) >> 1].min, greenminmaxtile[row - top][(col - left) >> 1].max);
              }
            }
          }
        }

        if(verbose) {printf("(%d)  thex[0][0]: %p  [", thexid, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thexid++;}
        for (int pass = 0; pass < passes; pass++) {
          if (pass == 1) {
            memcpy (rgb += 4, buffer, 4 * sizeof * rgb);
          }
          int thehexid2 = 1;
          if(verbose) {printf("(%d-%d-%d)  thex[0][0]: %p  [", thexid, pass, thehexid2, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thehexid2++;}
          /* Recalculate green from interpolated values of closer pixels: */
          if (pass) {
            for (int row = top + 2; row < mrow - 2; row++) {
              int leftstart = left + 2;

              for(; leftstart < mcol - 2; leftstart++)
                if(!isgreen(row, leftstart)) {
                  break;
                }

              int coloffset = (RightShift[row % 3] == 1 ? 3 : 1 + (fcol(row, leftstart + 1) & 1));

              if(coloffset == 3) {
                int f = fcol(row, leftstart);
                int *hex = allhex[1][row % 3][leftstart % 3];

                for (int col = leftstart; col < mcol - 2; col += coloffset, f ^= 2) {
                  for (int d = 3; d < 6; d++) {
                    float (*rix)[3] = &rgb[(d - 2)][row - top][col - left];
                    float val = 0.33333333f * (rix[-2 * hex[d]][1] + 2 * (rix[hex[d]][1] - rix[hex[d]][f])
                        - rix[-2 * hex[d]][f]) + rix[0][f];
                    rix[0][1] = LIM(val, greenminmaxtile[row - top][(col - left) >> 1].min, greenminmaxtile[row - top][(col - left) >> 1].max);
                  }
                }
              } else {
                int f = fcol(row, leftstart);
                int *hexmod[2];
                hexmod[0] = allhex[1][row % 3][leftstart % 3];
                hexmod[1] = allhex[1][row % 3][(leftstart + coloffset) % 3];

                for (int col = leftstart, hexindex = 0; col < mcol - 2; col += coloffset, coloffset ^= 3, f = f ^ (coloffset & 2), hexindex ^= 1 ) {
                  int *hex = hexmod[hexindex];

                  for (int d = 3; d < 6; d++) {
                    float (*rix)[3] = &rgb[(d - 2) ^ 1][row - top][col - left];
                    float val = 0.33333333f * (rix[-2 * hex[d]][1] + 2 * (rix[hex[d]][1] - rix[hex[d]][f])
                        - rix[-2 * hex[d]][f]) + rix[0][f];
                    rix[0][1] = LIM(val, greenminmaxtile[row - top][(col - left) >> 1].min, greenminmaxtile[row - top][(col - left) >> 1].max);
                  }
                }
              }
            }
          }

          if(verbose) {printf("(%d-%d-%d)  thex[0][0]: %p  [", thexid, pass, thehexid2, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thehexid2++;}
          /* Interpolate red and blue values for solitary green pixels:   */
          int sgstartrow = (top - sgrow + 4) / 3 * 3 + sgrow;
          int sgstartcol = (left - sgcol + 4) / 3 * 3 + sgcol;

          if( verbose ) {
            printf("sgstartrow=%d  sgstartcol=%d\n", sgstartrow, sgstartcol);
            printf("CFA pattern:\n");
            for (int row = 0; row < 16; row++) {
              for (int col = 0; col < 16; col++) {
                printf("%d ", (int)fcol(row+sgstartrow,col+sgstartcol));
              }
              printf("\n");
            }

          }

          for (int row = sgstartrow; row < mrow - 2; row += 3) {
            for (int col = sgstartcol, h = fcol(row, col + 1); col < mcol - 2; col += 3, h ^= 2) {
              float (*rix)[3] = &rgb[0][row - top][col - left];
              float diff[6] = {0.f};

              if(verbose) {printf("top=%d  left=%d  row=%d  col=%d  h=%d\n", winy, winx, row, col, h);}
              if( verbose && h == 1 ) {
                printf("CFA pattern:\n");
                for (int row2 = 0; row2 < 18; row2++) {
                  for (int col2 = 0; col2 < 18; col2++) {
                    printf("%d ", (int)fcol(row+row2-6,col+col2-6));
                  }
                  printf("\n");
                }
                getchar();
              }
              if(false && verbose) {printf("(%d-%d-%d, %d-%d)  thex[0][0]: %p  [", thexid, pass, thehexid2, row, col, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n");}
              for (int i = 1, d = 0; d < 6; d++, i ^= ts ^ 1, h ^= 2) {
                for (int c = 0; c < 2; c++, h ^= 2) {
                  float g = rix[0][1] + rix[0][1] - rix[i << c][1] - rix[-i << c][1];
                  if(false && verbose) {printf("(%d-%d-%d, %d-%d, %d-%d-%d-%d)  thex[0][0]: %p  [",
                      thexid, pass, thehexid2, row, col, i, d, c, h, (void*)(thex));
                  for(int cc=0; cc<8; cc++) printf("%d ",thex[cc]); printf("] before color[%d][%d]\n",h,d);}
                  color[h][d] = g + rix[i << c][h] + rix[-i << c][h];
                  if(false && verbose) {printf("(%d-%d-%d, %d-%d, %d-%d-%d-%d)  thex[0][0]: %p  [",
                      thexid, pass, thehexid2, row, col, i, d, c, h, (void*)(thex));
                  for(int cc=0; cc<8; cc++) printf("%d ",thex[cc]); printf("] after color[%d][%d]\n",h,d);}

                  if (d > 1)
                    diff[d] += SQR (rix[i << c][1] - rix[-i << c][1]
                                                                  - rix[i << c][h] + rix[-i << c][h]) + SQR(g);
                  if(false && verbose) {printf("(%d-%d-%d, %d-%d, %d-%d-%d-%d)  thex[0][0]: %p  [",
                      thexid, pass, thehexid2, row, col, i, d, c, h, (void*)(thex));
                  for(int cc=0; cc<8; cc++) printf("%d ",thex[cc]); printf("]\n");}
                }

                if (d > 2 && (d & 1))    // 3, 5
                    if (diff[d - 1] < diff[d])
                      for(int c = 0; c < 2; c++) {
                        color[c * 2][d] = color[c * 2][d - 1];
                      }

                if(false && verbose) {printf("(%d-%d-%d, %d-%d, %d-%d)  thex[0][0]: %p  [",
                    thexid, pass, thehexid2, row, col, i, d, (void*)(thex));
                for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n");}

                if ((d & 1) || d < 2) { // d: 0, 1, 3, 5
                  for(int c = 0; c < 2; c++) {
                    rix[0][c * 2] = CLIP(0.5f * color[c * 2][d]);
                  }

                  rix += ts * ts;
                }
                if(false && verbose) {printf("(%d-%d-%d, %d-%d, %d-%d)  thex[0][0]: %p  [",
                    thexid, pass, thehexid2, row, col, i, d, (void*)(thex));
                for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n");}
              }
              if(false && verbose) {printf("(%d-%d-%d, %d-%d)  thex[0][0]: %p  [", thexid, pass, thehexid2, row, col, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n");}
            }
          }

          if(verbose) {printf("(%d-%d-%d)  thex[0][0]: %p  [", thexid, pass, thehexid2, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thehexid2++;}
          /* Interpolate red for blue pixels and vice versa:      */
          for (int row = top + 3; row < mrow - 3; row++) {
            int leftstart = left + 3;

            for(; leftstart < mcol - 1; leftstart++)
              if(!isgreen(row, leftstart)) {
                break;
              }

            int coloffset = (RightShift[row % 3] == 1 ? 3 : 1);
            c = (row - sgrow) % 3 ? ts : 1;
            int h = 3 * (c ^ ts ^ 1);

            if(coloffset == 3) {
              int f = 2 - fcol(row, leftstart);

              for (int col = leftstart; col < mcol - 3; col += coloffset, f ^= 2) {
                float (*rix)[3] = &rgb[0][row - top][col - left];

                for (int d = 0; d < 4; d++, rix += ts * ts) {
                  int i = d > 1 || ((d ^ c) & 1) ||
                      ((fabsf(rix[0][1] - rix[c][1]) + fabsf(rix[0][1] - rix[-c][1])) < 2.f * (fabsf(rix[0][1] - rix[h][1]) + fabsf(rix[0][1] - rix[-h][1]))) ? c : h;

                  rix[0][f] = CLIP(rix[0][1] + 0.5f * (rix[i][f] + rix[-i][f] - rix[i][1] - rix[-i][1]));
                }
              }
            } else {
              coloffset = fcol(row, leftstart + 1) == 1 ? 2 : 1;
              int f = 2 - fcol(row, leftstart);

              for (int col = leftstart; col < mcol - 3; col += coloffset, coloffset ^= 3, f = f ^ (coloffset & 2) ) {
                float (*rix)[3] = &rgb[0][row - top][col - left];

                for (int d = 0; d < 4; d++, rix += ts * ts) {
                  int i = d > 1 || ((d ^ c) & 1) ||
                      ((fabsf(rix[0][1] - rix[c][1]) + fabsf(rix[0][1] - rix[-c][1])) < 2.f * (fabsf(rix[0][1] - rix[h][1]) + fabsf(rix[0][1] - rix[-h][1]))) ? c : h;

                  rix[0][f] = CLIP(rix[0][1] + 0.5f * (rix[i][f] + rix[-i][f] - rix[i][1] - rix[-i][1]));
                }
              }
            }
          }

          if(verbose) {printf("(%d-%d-%d)  thex[0][0]: %p  [", thexid, pass, thehexid2, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thehexid2++;}
          /* Fill in red and blue for 2x2 blocks of green:        */
          // Find first row of 2x2 green
          int topstart = top + 2;

          for(; topstart < mrow - 2; topstart++)
            if((topstart - sgrow) % 3) {
              break;
            }

          int leftstart = left + 2;

          for(; leftstart < mcol - 2; leftstart++)
            if((leftstart - sgcol) % 3) {
              break;
            }

          int coloffsetstart = 2 - (fcol(topstart, leftstart + 1) & 1);

          if(verbose) {printf("(%d-%d-%d)  thex[0][0]: %p  [", thexid, pass, thehexid2, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thehexid2++;}
          for (int row = topstart; row < mrow - 2; row++) {
            if ((row - sgrow) % 3) {
              int *hexmod[2];
              hexmod[0] = allhex[1][row % 3][leftstart % 3];
              hexmod[1] = allhex[1][row % 3][(leftstart + coloffsetstart) % 3];

              for (int col = leftstart, coloffset = coloffsetstart, hexindex = 0; col < mcol - 2; col += coloffset, coloffset ^= 3, hexindex ^= 1) {
                float (*rix)[3] = &rgb[0][row - top][col - left];
                int *hex = hexmod[hexindex];

                for (int d = 0; d < ndir; d += 2, rix += ts * ts) {
                  if (hex[d] + hex[d + 1]) {
                    float g = 3 * rix[0][1] - 2 * rix[hex[d]][1] - rix[hex[d + 1]][1];

                    for (c = 0; c < 4; c += 2) {
                      rix[0][c] = CLIP((g + 2 * rix[hex[d]][c] + rix[hex[d + 1]][c]) * 0.33333333f);
                    }
                  } else {
                    float g = 2 * rix[0][1] - rix[hex[d]][1] - rix[hex[d + 1]][1];

                    for (c = 0; c < 4; c += 2) {
                      rix[0][c] = CLIP((g + rix[hex[d]][c] + rix[hex[d + 1]][c]) * 0.5f);
                    }
                  }
                }
              }
            }
          }
          if(verbose) {printf("(%d-%d-%d)  thex[0][0]: %p  [", thexid, pass, thehexid2, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thehexid2++;}
        }

        if(verbose) {printf("(%d)  thex[0][0]: %p  [", thexid, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thexid++;}
        // end of multipass part
        rgb = (float(*)[ts][ts][3]) buffer;
        mrow -= top;
        mcol -= left;

        if(useCieLab) {
#ifdef HAS_CIELAB
          /* Convert to CIELab and differentiate in all directions:   */
          // Original dcraw algorithm uses CIELab as perceptual space
          // (presumably coming from original AHD) and converts taking
          // camera matrix into account.  We use this in RT.
          for (int d = 0; d < ndir; d++) {
            float *l = &lab[0][0][0];
            float *a = &lab[1][0][0];
            float *b = &lab[2][0][0];
            cielab(&rgb[d][4][4], l, a, b, ts, mrow - 8, ts - 8, xyz_cam);
            int f = dir[d & 3];
            f = f == 1 ? 1 : f - 8;

            for (int row = 5; row < mrow - 5; row++)
              //#ifdef _OPENMP
              //#pragma omp simd
              //#endif
              for (int col = 5; col < mcol - 5; col++) {
                float *l = &lab[0][row - 4][col - 4];
                float *a = &lab[1][row - 4][col - 4];
                float *b = &lab[2][row - 4][col - 4];

                float g = 2 * l[0] - l[f] - l[-f];
                drv[d][row - 5][col - 5] =  SQR(g) + SQR((2 * a[0] - a[f] - a[-f] + g * 2.1551724f))
                  + SQR((2 * b[0] - b[f] - b[-f] - g * 0.86206896f));
              }
          }
#endif
        } else {
          // For 1-pass demosaic we use YPbPr which requires much
          // less code and is nearly indistinguishable. It assumes the
          // camera RGB is roughly linear.
          for (int d = 0; d < ndir; d++) {
            float (*yuv)[ts - 8][ts - 8] = lab; // we use the lab buffer, which has the same dimensions
#ifdef __SSE2__
            vfloat zd2627v = F2V(0.2627f);
            vfloat zd6780v = F2V(0.6780f);
            vfloat zd0593v = F2V(0.0593f);
            vfloat zd56433v = F2V(0.56433f);
            vfloat zd67815v = F2V(0.67815f);
#endif

            for (int row = 4; row < mrow - 4; row++) {
              int col = 4;
#ifdef __SSE2__

              for (; col < mcol - 7; col += 4) {
                // use ITU-R BT.2020 YPbPr, which is great, but could use
                // a better/simpler choice? note that imageop.h provides
                // dt_iop_RGB_to_YCbCr which uses Rec. 601 conversion,
                // which appears less good with specular highlights
                vfloat redv, greenv, bluev;
                vconvertrgbrgbrgbrgb2rrrrggggbbbb(rgb[d][row][col], redv, greenv, bluev);
                vfloat yv = zd2627v * redv + zd6780v * bluev + zd0593v * greenv;
                STVFU(yuv[0][row - 4][col - 4], yv);
                STVFU(yuv[1][row - 4][col - 4], (bluev - yv) * zd56433v);
                STVFU(yuv[2][row - 4][col - 4], (redv - yv) * zd67815v);
              }

#endif

              for (; col < mcol - 4; col++) {
                // use ITU-R BT.2020 YPbPr, which is great, but could use
                // a better/simpler choice? note that imageop.h provides
                // dt_iop_RGB_to_YCbCr which uses Rec. 601 conversion,
                // which appears less good with specular highlights
                float y = 0.2627f * rgb[d][row][col][0] + 0.6780f * rgb[d][row][col][1] + 0.0593f * rgb[d][row][col][2];
                yuv[0][row - 4][col - 4] = y;
                yuv[1][row - 4][col - 4] = (rgb[d][row][col][2] - y) * 0.56433f;
                yuv[2][row - 4][col - 4] = (rgb[d][row][col][0] - y) * 0.67815f;
              }
            }

            int f = dir[d & 3];
            f = f == 1 ? 1 : f - 8;

            for (int row = 5; row < mrow - 5; row++) {
              for (int col = 5; col < mcol - 5; col++) {
                float *y = &yuv[0][row - 4][col - 4];
                float *u = &yuv[1][row - 4][col - 4];
                float *v = &yuv[2][row - 4][col - 4];
                drv[d][row - 5][col - 5] = SQR(2 * y[0] - y[f] - y[-f])
                                                             + SQR(2 * u[0] - u[f] - u[-f])
                                                             + SQR(2 * v[0] - v[f] - v[-f]);
              }
            }
          }
        }

        /* Build homogeneity maps from the derivatives:         */
#ifdef __SSE2__
        vfloat eightv = F2V(8.f);
        vfloat zerov = F2V(0.f);
        vfloat onev = F2V(1.f);
#endif

        for (int row = 6; row < mrow - 6; row++) {
          int col = 6;
#ifdef __SSE2__

          for (; col < mcol - 9; col += 4) {
            vfloat tr1v = vminf(LVFU(drv[0][row - 5][col - 5]), LVFU(drv[1][row - 5][col - 5]));
            vfloat tr2v = vminf(LVFU(drv[2][row - 5][col - 5]), LVFU(drv[3][row - 5][col - 5]));

            if(ndir > 4) {
              vfloat tr3v = vminf(LVFU(drv[4][row - 5][col - 5]), LVFU(drv[5][row - 5][col - 5]));
              vfloat tr4v = vminf(LVFU(drv[6][row - 5][col - 5]), LVFU(drv[7][row - 5][col - 5]));
              tr1v = vminf(tr1v, tr3v);
              tr1v = vminf(tr1v, tr4v);
            }

            tr1v = vminf(tr1v, tr2v);
            tr1v = tr1v * eightv;

            for (int d = 0; d < ndir; d++) {
              uint8_t tempstore[16];
              vfloat tempv = zerov;

              for (int v = -1; v <= 1; v++) {
                for (int h = -1; h <= 1; h++) {
                  tempv += vselfzero(vmaskf_le(LVFU(drv[d][row + v - 5][col + h - 5]), tr1v), onev);
                }
              }

              _mm_storeu_si128((__m128i*)&tempstore, _mm_cvtps_epi32(tempv));
              homo[d][row][col] = tempstore[0];
              homo[d][row][col + 1] = tempstore[4];
              homo[d][row][col + 2] = tempstore[8];
              homo[d][row][col + 3] = tempstore[12];

            }
          }

#endif

          for (; col < mcol - 6; col++) {
            float tr = drv[0][row - 5][col - 5] < drv[1][row - 5][col - 5] ? drv[0][row - 5][col - 5] : drv[1][row - 5][col - 5];

            for (int d = 2; d < ndir; d++) {
              tr = (drv[d][row - 5][col - 5] < tr ? drv[d][row - 5][col - 5] : tr);
            }

            tr *= 8;

            for (int d = 0; d < ndir; d++) {
              uint8_t temp = 0;

              for (int v = -1; v <= 1; v++) {
                for (int h = -1; h <= 1; h++) {
                  temp += (drv[d][row + v - 5][col + h - 5] <= tr ? 1 : 0);
                }
              }

              homo[d][row][col] = temp;
            }
          }
        }

        if (height - top < ts + 4) {
          mrow = height - top + 2;
        }

        if (width - left < ts + 4) {
          mcol = width - left + 2;
        }


        /* Build 5x5 sum of homogeneity maps */
        const int startcol = MIN(left, 8);

        for(int d = 0; d < ndir; d++) {
          for (int row = MIN(top, 8); row < mrow - 8; row++) {
            int col = startcol;
#ifdef __SSE2__
            int endcol = row < mrow - 9 ? mcol - 8 : mcol - 23;

            // crunching 16 values at once is faster than summing up column sums
            for (; col < endcol; col += 16) {
              vint v5sumv = (vint)ZEROV;

              for(int v = -2; v <= 2; v++)
                for(int h = -2; h <= 2; h++) {
                  v5sumv = _mm_adds_epu8( _mm_loadu_si128((vint*)&homo[d][row + v][col + h]), v5sumv);
                }

              _mm_storeu_si128((vint*)&homosum[d][row][col], v5sumv);
            }

#endif

            if(col < mcol - 8) {
              int v5sum[5] = {0};

              for(int v = -2; v <= 2; v++)
                for(int h = -2; h <= 2; h++) {
                  v5sum[2 + h] += homo[d][row + v][col + h];
                }

              int blocksum = v5sum[0] + v5sum[1] + v5sum[2] + v5sum[3] + v5sum[4];
              homosum[d][row][col] = blocksum;
              col++;

              // now we can subtract a column of five from blocksum and get new colsum of 5
              for (int voffset = 0; col < mcol - 8; col++, voffset++) {
                int colsum = homo[d][row - 2][col + 2] + homo[d][row - 1][col + 2] + homo[d][row][col + 2] + homo[d][row + 1][col + 2] + homo[d][row + 2][col + 2];
                voffset = voffset == 5 ? 0 : voffset;  // faster than voffset %= 5;
                blocksum -= v5sum[voffset];
                blocksum += colsum;
                v5sum[voffset] = colsum;
                homosum[d][row][col] = blocksum;
              }
            }
          }
        }

        // calculate maximum of homogeneity maps per pixel. Vectorized calculation is a tiny bit faster than on the fly calculation in next step
#ifdef __SSE2__
        vint maskv = _mm_set1_epi8(31);
#endif

        for (int row = MIN(top, 8); row < mrow - 8; row++) {
          int col = startcol;
#ifdef __SSE2__
          int endcol = row < mrow - 9 ? mcol - 8 : mcol - 23;

          for (; col < endcol; col += 16) {
            vint maxval1 = _mm_max_epu8(_mm_loadu_si128((vint*)&homosum[0][row][col]), _mm_loadu_si128((vint*)&homosum[1][row][col]));
            vint maxval2 = _mm_max_epu8(_mm_loadu_si128((vint*)&homosum[2][row][col]), _mm_loadu_si128((vint*)&homosum[3][row][col]));

            if(ndir > 4) {
              vint maxval3 = _mm_max_epu8(_mm_loadu_si128((vint*)&homosum[4][row][col]), _mm_loadu_si128((vint*)&homosum[5][row][col]));
              vint maxval4 = _mm_max_epu8(_mm_loadu_si128((vint*)&homosum[6][row][col]), _mm_loadu_si128((vint*)&homosum[7][row][col]));
              maxval1 = _mm_max_epu8(maxval1, maxval3);
              maxval1 = _mm_max_epu8(maxval1, maxval4);
            }

            maxval1 = _mm_max_epu8(maxval1, maxval2);
            // there is no shift intrinsic for epu8. Shift using epi32 and mask the wrong bits out
            vint subv = _mm_srli_epi32( maxval1, 3 );
            subv = _mm_and_si128(subv, maskv);
            maxval1 = _mm_subs_epu8(maxval1, subv);
            _mm_storeu_si128((vint*)&homosummax[row][col], maxval1);
          }

#endif

          for (; col < mcol - 8; col ++) {
            uint8_t maxval = homosum[0][row][col];

            for(int d = 1; d < ndir; d++) {
              maxval = maxval < homosum[d][row][col] ? homosum[d][row][col] : maxval;
            }

            maxval -= maxval >> 3;
            homosummax[row][col] = maxval;
          }
        }


        if(verbose) {printf("(%d)  thex[0][0]: %p  [", thexid, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thexid++;}
        /* Average the most homogeneous pixels for the final result: */
        uint8_t hm[8];

        for (int row = MIN(top, 8); row < mrow - 8; row++) {
          for (int col = MIN(left, 8); col < mcol - 8; col++) {
            int d = 0;

            for (; d < 4; d++) {
              hm[d] = homosum[d][row][col];
            }

            for (; d < ndir; d++) {
              hm[d] = homosum[d][row][col];

              if (hm[d - 4] < hm[d]) {
                hm[d - 4] = 0;
              } else if (hm[d - 4] > hm[d]) {
                hm[d] = 0;
              }
            }

            float avg[4] = {0.f};

            uint8_t maxval = homosummax[row][col];

            for (d = 0; d < ndir; d++)
              if (hm[d] >= maxval) {
                FORC3 avg[c] += rgb[d][row][col][c];
                avg[3]++;
              }

            red[row + top][col + left] = avg[0] / avg[3];
            green[row + top][col + left] = avg[1] / avg[3];
            blue[row + top][col + left] = avg[2] / avg[3];
            //std::cout<<"X-trans demo: setting output pixel "<<row+top<<","<<col+left<<": "<<avg[0] / avg[3]<<","<<avg[1] / avg[3]<<","<<avg[2] / avg[3]<<std::endl;
          }
        }
        if(verbose) {printf("(%d)  thex[0][0]: %p  [", thexid, (void*)(thex)); for(int c=0; c<8; c++) printf("%d ",thex[c]); printf("]\n"); thexid++;}
      }

    free(buffer);
  }
}

}
