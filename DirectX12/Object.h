#pragma once

class Object {
protected:
    char name_[16];

public:
    Object() {
        strcpy_s<16>(name_, "Object");
    }
    virtual ~Object() {}
    const char* getName() const { return name_; }

    virtual void draw(HDC hDC) = 0;
};
