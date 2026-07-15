#pragma once

class ISubMenuController
{
    public:
        virtual ~ISubMenuController() = default;

        virtual void run() = 0;
};
