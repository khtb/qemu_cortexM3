#pragma once

namespace Core
{
class IApp
{
  public:
    virtual ~IApp() = default;
    virtual int run() = 0;
};
} // namespace Core
