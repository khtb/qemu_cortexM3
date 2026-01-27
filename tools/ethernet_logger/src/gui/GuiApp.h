#pragma once
#include "../core/AppConfig.h"
#include "../core/IApp.h"

namespace GUI
{
class GuiApp : public Core::IApp
{
  public:
    explicit GuiApp(const Core::AppConfig &config);
    int run() override;

  private:
    Core::AppConfig m_config;
};
} // namespace GUI
