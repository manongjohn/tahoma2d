#include "texception.h"
#include "tfxparam.h"
#include "trop.h"
#include "stdfx.h"
#include "trasterfx.h"

//-------------------------------------------------------------------

class AntialiasFx final : public TStandardRasterFx {
  FX_PLUGIN_DECLARATION(AntialiasFx)

  TRasterFxPort m_input;
  TIntParamP m_threshold;
  TIntParamP m_softness;

public:
  AntialiasFx() : m_threshold(10), m_softness(100) {
    // threshold parameter (0-255)
    bindParam(this, "threshold", m_threshold);
    m_threshold->setValueRange(0, 255);
    m_threshold->enableSlider(true);

    // softness parameter (0-100)
    bindParam(this, "softness", m_softness);
    m_softness->setValueRange(0, 100);
    m_softness->enableSlider(true);

    // input port
    addInputPort("Source", m_input);
  }

  ~AntialiasFx(){};

  std::string getAlias(double frame, const TRenderSettings &info) const {
    if (!m_input.isConnected()) return std::string();

    int threshold = (int)m_threshold->getValue();
    int softness  = (int)m_softness->getValue();

    return getFxType() + "[" + m_input.getFx()->getAlias(frame, info) + "," +
           std::to_string(threshold) + "," + std::to_string(softness) + "]";
  }

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
    int threshold = (int)m_threshold->getValue();
    int softness  = (int)m_softness->getValue();

    // First compute the input w/o affine (scale, shear, rotation) applied.
    TTile tileIn;
    TRenderSettings aaRi(ri);
    aaRi.m_disableAntiAlias = true;
    aaRi.m_affine           = handledAffine(ri, frame);
    TAffine appliedAff      = ri.m_affine * aaRi.m_affine.inv();

    // Tile size needs to account for image w/ and w/o affine applied.
    TRectD bbox, riBbox, aaRiBbox;
    m_input->getBBox(frame, riBbox, ri);
    m_input->getBBox(frame, aaRiBbox, aaRi);
    bbox = riBbox + aaRiBbox;
    m_input->allocateAndCompute(tileIn, bbox.getP00(),
                                TDimension(bbox.getLx(), bbox.getLy()),
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
    aaRi.m_affine = appliedAff;
    TRasterFx::applyAffine(tile, tileIn, aaRi);
  }

  bool canHandle(const TRenderSettings &info, double frame) override {
    // TODO: Is this needed?
    return true;
  }
};

// Register the FX
FX_PLUGIN_IDENTIFIER(AntialiasFx, "antialiasFx")