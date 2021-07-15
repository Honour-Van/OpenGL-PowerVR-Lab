#pragma once
#include "PVRShell/PVRShell.h"
#include "PVRUtils/PVRUtilsGles.h"
#include <stack>

static const float cubesize = 0.03f; // Cube size
constexpr int _boardLen = 16;
int board[2*_boardLen+1][2*_boardLen+1];
int &checkBoard(int x, int z) { return board[x+_boardLen][z+_boardLen];}
GLfloat delta = 1.f/_boardLen;

class Triangle
{
protected:
    // VBO handle
    uint32_t _vbo;

    // Texture handle
    uint32_t _texture;

    // MVP Matrix location
    uint32_t _mvp;

    // Matrix for the Model position
    glm::mat4 _position;

    // Matrix for the Model rotation
    glm::mat4 _rotation;

public:
    Triangle(void);
    ~Triangle(void);

    bool Init(pvr::Shell *shell, uint32_t mvpLoc);
    void Render(glm::mat4 view, glm::mat4 projection);
    void SetPosition(float x, float y, float z);
};

class Cube : public Triangle
{
public:
    bool Init(pvr::Shell *shell, uint32_t mvpLoc, GLuint color=7);
    void Render(glm::mat4 view, glm::mat4 projection);
};

class mBox : public Cube
{
private:
    int _direction; // 0 forward(z++) 1 right(x++) 2 backward(z--) 3 left(x--)

public:
    int GetDirec() { return _direction; };
    void SetDirec(int dirc) { _direction = dirc; };
};