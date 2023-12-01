#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>

#include <jsi/jsi.h>

#include "JsiValueWrapper.h"
#include "RNSkView.h"

#include "JsiSkPicture.h"
#include "RNSkInfoParameter.h"
#include "RNSkLog.h"
#include "RNSkPlatformContext.h"
#include "RNSkTimingInfo.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"

#include "SkBBHFactory.h"
#include "SkCanvas.h"
#include "SkPictureRecorder.h"
#include <modules/skottie/include/Skottie.h>

#pragma clang diagnostic pop

class SkRect;

namespace RNSkia {

namespace jsi = facebook::jsi;

class RNSkSkottieRenderer
    : public RNSkRenderer,
      public std::enable_shared_from_this<RNSkSkottieRenderer> {
public:
  RNSkSkottieRenderer(std::function<void()> requestRedraw,
                      std::shared_ptr<RNSkPlatformContext> context)
      : RNSkRenderer(requestRedraw), _platformContext(context) {}

  bool tryRender(std::shared_ptr<RNSkCanvasProvider> canvasProvider) override {
    return performDraw(canvasProvider);
  }

  void
  renderImmediate(std::shared_ptr<RNSkCanvasProvider> canvasProvider) override {
    performDraw(canvasProvider);
  }
          
  void setSrc(std::string src) {
    _animation = skottie::Animation::Builder().make(src.c_str(), src.size());
    animationStart = CACurrentMediaTime();
  }

private:
  bool performDraw(std::shared_ptr<RNSkCanvasProvider> canvasProvider) {
    canvasProvider->renderToCanvas([=](SkCanvas *canvas) {
      // Make sure to scale correctly
      auto pd = _platformContext->getPixelDensity();
      canvas->clear(SK_ColorTRANSPARENT);
      canvas->save();
      canvas->scale(pd, pd);

      if (_animation != nullptr) {
          auto now = CACurrentMediaTime();
          double elapsedTimeInMilliseconds = (now - animationStart) * 1000.0;
          double animationDuration = _animation.get()->duration() * 1000.0;
          double progress = std::fmod(elapsedTimeInMilliseconds, animationDuration) / animationDuration;

          _animation.get()->render(canvas);
          _animation.get()->seek(progress);
      }

      canvas->restore();
    });
    return true;
  }

  std::shared_ptr<RNSkPlatformContext> _platformContext;
  sk_sp<skottie::Animation> _animation;
  CFTimeInterval animationStart;
};

class RNSkSkottieView : public RNSkView {
public:
  /**
   * Constructor
   */
  RNSkSkottieView(std::shared_ptr<RNSkPlatformContext> context,
                  std::shared_ptr<RNSkCanvasProvider> canvasProvider)
      : RNSkView(
            context, canvasProvider,
            std::make_shared<RNSkSkottieRenderer>(
                std::bind(&RNSkSkottieView::requestRedraw, this), context)) {}

  void setJsiProperties(
      std::unordered_map<std::string, RNJsi::JsiValueWrapper> &props) override {

    RNSkView::setJsiProperties(props);

    for (auto &prop : props) {
        if (prop.first == "src" && prop.second.getType() == RNJsi::JsiWrapperValueType::String) {
            std::static_pointer_cast<RNSkSkottieRenderer>(getRenderer())
            ->setSrc(prop.second.getAsString());
        }
    }
  }
};
} // namespace RNSkia
