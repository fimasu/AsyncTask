#pragma once

// --------------------------------------------------------------

namespace base
{

// --------------------------------------------------------------

class IAwaiter
{
public:
    IAwaiter()          = default;
    virtual ~IAwaiter() = default;

    virtual bool done() const   = 0;
    virtual void resume()       = 0;
};

// --------------------------------------------------------------

} // namespace base