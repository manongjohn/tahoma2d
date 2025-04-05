#include "texception.h"
#include "tfxparam.h"
#include "trop.h"
#include "stdfx.h"
#include "trasterfx.h"

//-------------------------------------------------------------------

class AntialiasFx final : public TStandardRasterFx {
  FX_PLUGIN_DECLARATION(AntialiasFx)

  TRasterFxPort m_input;
  TDoubleParamP m_threshold;
  TDoubleParamP m_softness;

public:
  AntialiasFx() : m_threshold(10), m_softness(100) {
    // threshold parameter (0-256)
    bindParam(this, "threshold", m_threshold);
    m_threshold->setValueRange(0, 256);

    // softness parameter (0-100)
    bindParam(this, "softness", m_softness);
    m_softness->setValueRange(0, 100);

    // input port
    addInputPort("Source", m_input);
  }

  ~AntialiasFx(){};

  bool doGetBBox(double frame, TRectD &bBox,
                 const TRenderSettings &info) override {
    // Antialias doesn't change the bounding box...
    if (m_input.isConnected()) {
      return m_input->doGetBBox(frame, bBox, info);
    } else {
      bBox = TRectD();
      return false;
    }
  }

  void transform(double frame, int port, const TRectD &rectOnOutput,
                 const TRenderSettings &infoOnOutput, TRectD &rectOnInput,
                 TRenderSettings &infoOnInput) override {
    // Antialias doesn't transform the coordinates...
    infoOnInput = infoOnOutput;
    rectOnInput = rectOnOutput;
  }

  void doCompute(TTile &tile, double frame,
                 const TRenderSettings &ri) override {
    if (!m_input.isConnected()) return;

    // Get the param values
    int threshold = (int)m_threshold->getValue(frame);
    int softness  = (int)m_softness->getValue(frame);

    // Skip if no softness...
    if (softness == 0) {
      m_input->compute(tile, frame, ri);
      return;
    }

    // First compute the input into our tile...

    TRenderSettings aaRi(ri);
    aaRi.m_affine = handledAffine(ri, frame);

    TTile tileIn;
    double lx = tile.getRaster()->getLx();
    double ly = tile.getRaster()->getLy();
    m_input->allocateAndCompute(tileIn, tile.m_pos,
                             TDimension(lx, ly),
                             tile.getRaster(), frame, aaRi);

    TRasterP ras = tileIn.getRaster();

    // Skip antialiasing for float rasters (same check as in tcolumnfx.cpp)
    if (TRasterFP rasF = ras) {
      return;
    }

    // Apply antialias
    TRasterP aaRas = ras->create(ras->getLx(), ras->getLy());
    TRop::antialias(ras, aaRas, threshold, softness);
    ras->copy(aaRas);


    // Apply Affine
    aaRi.m_affine = ri.m_affine * aaRi.m_affine.inv();
    TRasterFx::applyAffine(tile, tileIn, aaRi);
  }

  bool canHandle(const TRenderSettings &info, double frame) override {
    // TODO: Is this needed?
    return true;
  }
};

// Register the FX
FX_PLUGIN_IDENTIFIER(AntialiasFx, "antialiasFx")