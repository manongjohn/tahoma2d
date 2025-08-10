#pragma once

#ifndef MYPAINTTOONZBRUSH_H
#define MYPAINTTOONZBRUSH_H

#include <toonz/mypaint.h>
#include "traster.h"
#include "trastercm.h"
#include "tcurves.h"
#include "symmetrytool.h"
#include <QPainter>
#include <QImage>

class RasterController {
public:
  virtual ~RasterController() {}
  virtual bool askRead(const TRect &rect) { return true; }
  virtual bool askWrite(const TRect &rect) { return true; }
};

//=======================================================
//
// Raster32PMyPaintSurface
//
//=======================================================

class Raster32PMyPaintSurface : public mypaint::Surface {
private:
  class Internal;

  TRaster32P m_ras;
  RasterController *controller;
  Internal *internal;

  inline static void readPixel(const void *pixelPtr, float &colorR,
                               float &colorG, float &colorB, float &colorA) {
    const TPixel32 &pixel = *(const TPixel32 *)pixelPtr;
    colorR                = (float)pixel.r / (float)TPixel32::maxChannelValue;
    colorG                = (float)pixel.g / (float)TPixel32::maxChannelValue;
    colorB                = (float)pixel.b / (float)TPixel32::maxChannelValue;
    colorA                = (float)pixel.m / (float)TPixel32::maxChannelValue;
  }

  inline static void writePixel(void *pixelPtr, float colorR, float colorG,
                                float colorB, float colorA) {
    TPixel32 &pixel = *(TPixel32 *)pixelPtr;
    pixel.r = (TPixel32::Channel)roundf(colorR * TPixel32::maxChannelValue);
    pixel.g = (TPixel32::Channel)roundf(colorG * TPixel32::maxChannelValue);
    pixel.b = (TPixel32::Channel)roundf(colorB * TPixel32::maxChannelValue);
    pixel.m = (TPixel32::Channel)roundf(colorA * TPixel32::maxChannelValue);
  }

  inline static bool askRead(void *surfaceController,
                             const void * /* surfacePointer */, int x0, int y0,
                             int x1, int y1) {
    Raster32PMyPaintSurface &owner =
        *((Raster32PMyPaintSurface *)surfaceController);
    return !owner.controller ||
           owner.controller->askRead(TRect(x0, y0, x1, y1));
  }

  inline static bool askWrite(void *surfaceController,
                              const void * /* surfacePointer */, int x0, int y0,
                              int x1, int y1) {
    Raster32PMyPaintSurface &owner =
        *((Raster32PMyPaintSurface *)surfaceController);
    return !owner.controller ||
           owner.controller->askWrite(TRect(x0, y0, x1, y1));
  }

public:
  explicit Raster32PMyPaintSurface(const TRaster32P &ras);
  explicit Raster32PMyPaintSurface(const TRaster32P &ras,
                                   RasterController &controller);
  ~Raster32PMyPaintSurface();

  bool getColor(float x, float y, float radius, float &colorR, float &colorG,
                float &colorB, float &colorA) override;

  bool drawDab(const mypaint::Dab &dab) override;

  bool getAntialiasing() const;
  void setAntialiasing(bool value);
};

//=======================================================
//
// MyPaintToonzBrush
//
//=======================================================

class MyPaintToonzBrush {
private:
  struct Params {
    union {
      struct {
        double x, y, pressure, tiltX, tiltY, time;
      };
      struct {
        double values[6];
      };
    };

    inline explicit Params(double x = 0.0, double y = 0.0,
                           double pressure = 0.0, double tiltX = 0.0,
                           double tiltY = 0.0, double time = 0.0)
        : x(x)
        , y(y)
        , pressure(pressure)
        , tiltX(tiltX)
        , tiltY(tiltY)
        , time(time) {}

    inline void setMedian(Params &a, Params &b) {
      for (int i  = 0; i < (int)sizeof(values) / sizeof(values[0]); ++i)
        values[i] = 0.5 * (a.values[i] + b.values[i]);
    }
  };

  struct Segment {
    Params p1, p2;
  };

  TRaster32P m_ras;
  Raster32PMyPaintSurface m_mypaintSurface;

  // Symmetry tool
  mypaint::Brush brushes[MAX_SYMMETRY];
  int m_brushCount;
  double m_rotation;
  TPointD m_centerPoint;
  bool m_useLineSymmetry;
  TPointD m_dpiScale;
  TPointD m_rasCenter;

  bool reset;
  Params previous, current;

public:
  MyPaintToonzBrush(const TRaster32P &ras, RasterController &controller,
                    const mypaint::Brush &brush);
  void beginStroke();
  void endStroke();
  void strokeTo(const TPointD &p, double pressure, double tiltX, double tiltY,
                double dtime);

  // colormapped
  void updateDrawing(const TRasterCM32P rasCM, const TRasterCM32P rasBackupCM,
                     const TRect &bbox, int styleId, bool lockAlpha,
                     int drawOrderMode, QSet<int> aboveStyleIds) const;

  void updateDrawing(const TRaster32P ras, const TRaster32P rasBackup,
                     const TRect &bbox, bool paintBehind) const;

  void addSymmetryBrushes(double lines, double rotation, TPointD centerPoint,
                          bool useLineSymmetry, TPointD dpiScale);
  bool hasSymmetryBrushes() { return m_brushCount > 1; }
};

#endif  // T_BLUREDBRUSH
