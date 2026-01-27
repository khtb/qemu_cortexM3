#pragma once
#include "../core/AppConfig.h"
#include "../core/IApp.h"

namespace CLI
{
class CliApp : public Core::IApp
{
  public:
    explicit CliApp(const Core::AppConfig &config);
    int run() override;

  private:
    Core::AppConfig m_config;
};
} // namespace CLI
