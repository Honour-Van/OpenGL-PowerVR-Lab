#include "PVRShell/PVRShell.h"
#include "PVRUtils/PVRUtilsGles.h"
#include <vector>
#include <iostream>
#include <list>
#include <ctime>
#include <cstdlib>

/*!*********************************************************************************************************************
 To use the shell, you have to inherit a class from PVRShell
 and implement the five virtual functions which describe how your application initializes, runs and releases the resources.
***********************************************************************************************************************/
const float cubesize = 0.03f; // Cube size
constexpr int _boardLen = 16;
int board[4 * _boardLen + 1][4 * _boardLen + 1];
const float delta = 1.f / _boardLen;
int &checkBoard(int x, int z) { return board[x + 2 * _boardLen][z + 2 * _boardLen]; }

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
    void MovePace(float x, float y, float z);
    void Hide();
};

class Cube : public Triangle
{
public:
    bool Init(pvr::Shell *shell, uint32_t mvpLoc, GLuint color = 7);
    void Render(glm::mat4 view, glm::mat4 projection);
};

class mBox : public Cube
{
private:
    int _direction; // 0 forward(z++) 1 right(x++) 2 backward(z--) 3 left(x--)
    int _x, _z;

public:
    int GetDirec() { return _direction; };
    void SetDirec(int dirc) { _direction = dirc; };
    void SetPosition(int x, int z);
    void Move(float progress);
    void Move(int &, int &);
    void Hide();
};

class Treat : public Cube
{
    int x, y;

public:
    void generateTreat();
    void SetPosition() { Cube::SetPosition(x * delta, cubesize, y * delta); }
    bool getTreat(int x_, int y_) { return x == x_ && y == y_; }
};

unsigned seed = time(0);
void Treat::generateTreat()
{
    srand(seed);
    int nx, ny;
    do
    {
        nx = rand() % (2 * _boardLen) - _boardLen;
        ny = rand() % (2 * _boardLen) - _boardLen;
    } while (checkBoard(nx, ny));
    checkBoard(x, y) = 0;
    checkBoard(nx, ny) = 2;
    x = nx, y = ny;
}

class HelloPVR : public pvr::Shell
{
    pvr::EglContext _context;
    // UIRenderer used to display text
    pvr::ui::UIRenderer _uiRenderer;
    GLuint _program;
    GLuint _vbo;

    std::vector<Cube *> _cubes;

    // from previous head to the last but one (just before tail)
    std::list<mBox *> _snake;
    mBox *_snakeHead;
    mBox *_snakeTail;
    int curDirec;
    int curX, curZ; // base place of the head
    Treat _treat;

    bool atGrid = false;

    glm::vec3 _camPosition;
    glm::mat4 _projection;
    float _camTheta;
    float _camRho;

public:
    // following function must be override
    virtual pvr::Result initApplication();
    virtual pvr::Result initView();
    virtual pvr::Result renderFrame();
    virtual pvr::Result releaseView();
    virtual pvr::Result quitApplication();
};

/*!*********************************************************************************************************************
\return Return Result::Success if no error occurred
\brief  Code in initApplication() will be called by pvr::Shell once per run, before the rendering context is created.
    Used to initialize variables that are not dependent on it (e.g. external modules, loading meshes, etc.)
    If the rendering context is lost, initApplication() will not be called again.
***********************************************************************************************************************/
pvr::Result HelloPVR::initApplication()
{
    for (int i = -_boardLen; i <= _boardLen; i++)
        for (int j = -_boardLen; j <= _boardLen; j++)
        {
            Cube *tmp = new Cube;
            tmp->SetPosition(static_cast<float>(i) * delta, -cubesize,
                             static_cast<float>(j) * delta);
            _cubes.push_back(tmp);
        }

    curDirec = 2;
    _snakeHead = new mBox;
    _snakeHead->SetDirec(curDirec);
    _snakeHead->SetPosition(1, 0);
    curX = 1, curZ = -1;

    mBox *tmpp;
    for (int i = 0; i < 3; i++)
    {
        tmpp = new mBox;
        tmpp->SetDirec(curDirec);
        tmpp->SetPosition(1, i);
        _snake.push_back(tmpp);
        checkBoard(1, i) = 1; // body is here
    }

    _snakeTail = tmpp;

    _treat.generateTreat();
    _treat.SetPosition();

    return pvr::Result::Success;
}

/*!*********************************************************************************************************************
\return Return Result::Success if no error occurred
\brief  Code in quitApplication() will be called by pvr::Shell once per run, just before exiting the program.
        If the rendering context is lost, QuitApplication() will not be called.
***********************************************************************************************************************/
pvr::Result HelloPVR::quitApplication()
{
    return pvr::Result::Success;
}

/*!*********************************************************************************************************************
\return Return Result::Success if no error occured
\brief  Code in initView() will be called by pvr::Shell upon initialization or after a change in the rendering context.
    Used to initialize variables that are dependant on the rendering context (e.g. textures, vertex buffers, etc.)
***********************************************************************************************************************/
pvr::Result HelloPVR::initView()
{
    // Initialize the PowerVR OpenGL bindings. Must be called before using any of the gl:: commands.
    _context = pvr::createEglContext();
    _context->init(getWindow(), getDisplay(), getDisplayAttributes());

    // Setup the text to be rendered
    _uiRenderer.init(getWidth(), getHeight(), isFullScreen(), (_context->getApiVersion() == pvr::Api::OpenGLES2) || (getBackBufferColorspace() == pvr::ColorSpace::sRGB));
    _uiRenderer.getDefaultTitle()->setText("Gluttonous Snake");
    _uiRenderer.getDefaultTitle()->commitUpdates();

    static const char *attribs[] = {"inVertex", "inTexColor"};
    static const uint16_t attribIndices[] = {0, 1};

    _program = pvr::utils::createShaderProgram(*this, "VertShader.vsh",
                                               "FragShader.fsh", attribs, attribIndices, 2, 0, 0);

    // Store the location of uniforms for later use
    uint32_t mvpLoc = gl::GetUniformLocation(_program, "MVPMatrix");

    for (auto item : _cubes)
        if (!item->Init(this, mvpLoc, 4))
        {
            throw pvr::InvalidDataError(" ERROR: Triangle failed in Init()");
            return pvr::Result::UnknownError;
        }
    for (auto mbox : _snake)
        if (!mbox->Init(this, mvpLoc, 2))
        {
            throw pvr::InvalidDataError(" ERROR: Triangle failed in Init()");
            return pvr::Result::UnknownError;
        }
    if ((!_snakeHead->Init(this, mvpLoc, 2)) ||
        (!_snakeTail->Init(this, mvpLoc, 2)) ||
        (!_treat.Init(this, mvpLoc, 3)))
    {
        throw pvr::InvalidDataError(" ERROR: Triangle failed in Init()");
        return pvr::Result::UnknownError;
    }

    _camTheta = glm::radians(55.f);
    _camRho = 2.0f;
    _projection = pvr::math::perspective(pvr::Api::OpenGLES2, 45, static_cast<float>(this->getWidth()) / static_cast<float>(this->getHeight()), 0.1, 100, 0);

    // Sets the clear color
    gl::ClearColor(0.180f, 0.176f, 0.176f, 1.0f);

    // Enable culling
    gl::Enable(GL_CULL_FACE);
    return pvr::Result::Success;
}

/*!*********************************************************************************************************************
\return Return Result::Success if no error occurred
\brief  Code in releaseView() will be called by pvr::Shell when the application quits or before a change in the rendering context.
***********************************************************************************************************************/
pvr::Result HelloPVR::releaseView()
{
    _snakeHead->Hide();
    for (auto node : _snake)
        node->Hide();
    _snakeTail->Hide();
    while (!_snake.empty())
        _snake.pop_back();
    // Release Vertex buffer object.
    if (_vbo)
        gl::DeleteBuffers(1, &_vbo);

    // Frees the OpenGL handles for the program and the 2 shaders
    gl::DeleteProgram(_program);
    return pvr::Result::Success;
}

/*!*********************************************************************************************************************
\return Return Result::Success if no error occurred
\brief  Main rendering loop function of the program. The shell will call this function every frame.
***********************************************************************************************************************/
int nframe = 0;
const int cellFrame = 20;
bool _pause = false;
pvr::Result HelloPVR::renderFrame()
{
    //  Clears the color buffer. glClear() can also be used to clear the depth or stencil buffer
    //  (GL_DEPTH_BUFFER_BIT or GL_STENCIL_BUFFER_BIT)
    gl::Clear(GL_COLOR_BUFFER_BIT);

    gl::UseProgram(_program);

    if (pvr::Shell::isKeyPressed(pvr::Keys::Left))
        _camTheta += 0.01f;
    if (pvr::Shell::isKeyPressed(pvr::Keys::Right))
        _camTheta -= 0.01f;

    if (pvr::Shell::isKeyPressed(pvr::Keys::W))
        if (curDirec != 0) curDirec = 2;
    if (pvr::Shell::isKeyPressed(pvr::Keys::A))
        if (curDirec != 1) curDirec = 3;
    if (pvr::Shell::isKeyPressed(pvr::Keys::S))
        if (curDirec != 2) curDirec = 0;
    if (pvr::Shell::isKeyPressed(pvr::Keys::D))
        if (curDirec != 3) curDirec = 1;
    if (pvr::Shell::isKeyPressed(pvr::Keys::Comma))
        _pause = true;
    else if (pvr::Shell::isKeyPressed(pvr::Keys::Period))
        _pause = false;

    if (!_pause)
    {
        if (atGrid)
        {
            _snake.push_front(_snakeHead);

            _snakeHead = new mBox;
            _snakeHead->SetDirec(curDirec);
            _snake.front()->SetDirec(curDirec); // ???????????????????????????????????????????????????????????????
            _snakeHead->SetPosition(curX, curZ);
            _snakeHead->Move(curX, curZ);
            if (checkBoard(curX, curZ) == 1)
                _pause = true;
            uint32_t mvpLoc = gl::GetUniformLocation(_program, "MVPMatrix");
            _snakeHead->Init(this, mvpLoc, 2);

            _snakeTail->Hide();
            if (_treat.getTreat(curX, curZ))
            {
                _treat.generateTreat();
                _treat.SetPosition();
            }
            else
                _snake.pop_back(); // ???????????????treat??????????????????
            _snakeTail = _snake.back();

            atGrid = false;
        }
        else
        {
            _snakeHead->Move(cellFrame);
            _snakeTail->Move(cellFrame);
            nframe += 1;
            if (nframe == cellFrame)
                atGrid = true, nframe = 0;
        }
    }
    else /* when pausing*/ 
    if (pvr::Shell::isKeyPressed(pvr::Keys::Key9))
    {
        _pause = false;
        releaseView();
        initApplication();
        initView();
    }


    _camPosition = glm::vec3(_camRho * cos(_camTheta), 1, _camRho * sin(_camTheta));

    glm::mat4 view = glm::lookAt(_camPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    for (auto cube : _cubes)
        cube->Render(view, _projection);
    for (auto snake : _snake)
        snake->Render(view, _projection);
    _snakeHead->Render(view, _projection);
    _snakeTail->Render(view, _projection);
    _treat.Render(view, _projection);
    // Display some text
    _uiRenderer.beginRendering();
    _uiRenderer.getDefaultTitle()->render();
    _uiRenderer.endRendering();

    _context->swapBuffers();
    return pvr::Result::Success;
}

/// <summary>This function must be implemented by the user of the shell. The user should return its pvr::Shell object defining the behaviour of the application.</summary>
/// <returns>Return a unique ptr to the demo supplied by the user.</returns>
std::unique_ptr<pvr::Shell> pvr::newDemo()
{
    return std::unique_ptr<pvr::Shell>(new HelloPVR());
}

Triangle::Triangle(void) : _vbo(0), _texture(0), _mvp(0),
                           _position(glm::mat4(1.0f)), _rotation(glm::mat4(1.0f))
{
}

Triangle::~Triangle(void)
{
    // Release Vertex buffer object.
    if (_vbo)
        gl::DeleteBuffers(1, &_vbo);
    // Frees the texture
    if (_texture)
        gl::DeleteTextures(1, &_texture);
}

bool Triangle::Init(pvr::Shell *shell, uint32_t mvpLoc)
{
    GLfloat afVertices[] = {
        // Vertex 1
        -0.4f, -0.4f, -0.0f, // Position 1
        2.0f, 0.0f, 0.0f,    // Texture coodinate 1
        // Vertex 2
        0.4f, -0.4f, -0.0f, // Position 2
        2.0f, 1.0f, 0.0f,   // Texture coodinate 2
        // Vertex 3
        0.0f, 0.4f, -0.0f, // Position 3
        2.0f, 0.5f, 1.0f,  // Texture coodinate 3
    };

    // Create VBO for the triangle from our data
    // Gen VBO
    gl::GenBuffers(1, &_vbo);

    // Bind the VBO
    gl::BindBuffer(GL_ARRAY_BUFFER, _vbo);

    // Set the buffer's data
    gl::BufferData(GL_ARRAY_BUFFER, 3 * 6 * sizeof(GLfloat), afVertices, GL_STATIC_DRAW);

    // Unbind the VBO
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);

    // Load the diffuse texture map using PVR Utils
    _texture = pvr::utils::textureUpload(*shell, "Image.pvr", GL_TRUE);
    gl::BindTexture(GL_TEXTURE_2D, _texture);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Save the MVP matrix location for later use
    _mvp = mvpLoc;

    return true;
}

void Triangle::Render(glm::mat4 view, glm::mat4 projection)
{
    unsigned int _stride = 6 * sizeof(GLfloat);

    glm::mat4 model = _position * _rotation;

    gl::UniformMatrix4fv(_mvp, 1, GL_FALSE, glm::value_ptr(projection * view * model));

    // Bind the VBO
    gl::BindBuffer(GL_ARRAY_BUFFER, _vbo);

    gl::EnableVertexAttribArray(0);
    // Points to the position data
    gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, _stride, 0);
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, _stride, (void *)(3 * sizeof(GLfloat)));
    gl::DrawArrays(GL_TRIANGLES, 0, 3);

    // Unbind the VBO
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);
}

void Triangle::SetPosition(float x, float y, float z)
{
    _position = glm::translate(glm::vec3(x, y, z));
}

void Triangle::MovePace(float x, float y, float z)
{
    _position = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)) * _position;
}

bool Cube::Init(pvr::Shell *shell, uint32_t mvpLoc, GLuint color)
{
    static char vertices[] = {
        -1, -1, -1, -1, -1, 1, -1, 1, 1,
        -1, -1, -1, -1, 1, 1, -1, 1, -1, // Left

        1, 1, 1, 1, -1, -1, 1, 1, -1,
        1, -1, -1, 1, 1, 1, 1, -1, 1, // Right

        1, 1, -1, -1, -1, -1, -1, 1, -1,
        1, 1, -1, 1, -1, -1, -1, -1, -1, // Back

        -1, 1, 1, -1, -1, 1, 1, -1, 1,
        1, 1, 1, -1, 1, 1, 1, -1, 1, // Front

        1, 1, 1, 1, 1, -1, -1, 1, -1,
        1, 1, 1, -1, 1, -1, -1, 1, 1, // Top

        1, -1, 1, -1, -1, -1, 1, -1, -1,
        1, -1, 1, -1, -1, 1, -1, -1, -1, // Bottom
    };

    GLfloat vbodata[12 * 3 * 6];
    static const GLfloat facecolor[] =
        {
            0.0f, 1.0f, 0.0f, // Green
            0.0f, 0.0f, 1.0f, // Blue
            1.0f, 0.5f, 0.0f, // Oriange
            1.0f, 0.0f, 0.0f, // Red
            1.0f, 1.0f, 1.0f, // White
            1.0f, 1.0f, 0.0f, // Yellow
            0.0f, 0.0f, 0.0f, // Black
        };

    for (int i = 0; i < 6; ++i)
    {
        int colori = i == 4 ? 4 : 6;
        if (color != 7)
            colori = color;
        for (int j = 0; j < 6; ++j)
        {
            vbodata[i * 36 + j * 6] = vertices[i * 18 + j * 3] * cubesize;
            vbodata[i * 36 + j * 6 + 1] = vertices[i * 18 + j * 3 + 1] * cubesize;
            vbodata[i * 36 + j * 6 + 2] = vertices[i * 18 + j * 3 + 2] * cubesize;
            for (int k = 0; k < 3; ++k)
                vbodata[i * 36 + j * 6 + 3 + k] = facecolor[colori * 3 + k];
        }
    }
    gl::GenBuffers(1, &_vbo);
    gl::BindBuffer(GL_ARRAY_BUFFER, _vbo);
    gl::BufferData(GL_ARRAY_BUFFER, sizeof(vbodata), vbodata, GL_STATIC_DRAW);
    // Unbind the VBO
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);
    // Save the MVP matrix location for later use
    _mvp = mvpLoc;

    return true;
}

void Cube::Render(glm::mat4 view, glm::mat4 projection)
{
    unsigned int _stride = 6 * sizeof(GLfloat);

    glm::mat4 model = _position * _rotation;

    gl::UniformMatrix4fv(_mvp, 1, GL_FALSE, glm::value_ptr(projection * view * model));

    // Bind the VBO
    gl::BindBuffer(GL_ARRAY_BUFFER, _vbo);

    gl::EnableVertexAttribArray(0);
    // Points to the position data
    gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, _stride, 0);
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, _stride, (void *)(3 * sizeof(GLfloat)));
    gl::DrawArrays(GL_TRIANGLES, 0, 3 * 12);

    // Unbind the VBO
    gl::BindBuffer(GL_ARRAY_BUFFER, 0);
}

void mBox::SetPosition(int x, int z)
{
    Cube::SetPosition(delta * x, cubesize, delta * z);
    checkBoard(x, z) = 1;
    _x = x, _z = z; // set????????????????????????????????????????????????????????????????????????????????????
}

void mBox::Move(float cellframe)
{
    float pace = delta / cellframe;
    if (_direction == 0)
        Cube::MovePace(0, 0, pace);
    else if (_direction == 1)
        Cube::MovePace(pace, 0, 0);
    else if (_direction == 2)
        Cube::MovePace(0, 0, -pace);
    else if (_direction == 3)
        Cube::MovePace(-pace, 0, 0);
}

void mBox::Move(int &x, int &z)
{
    if (_direction == 0)
        z += 1;
    else if (_direction == 1)
        x += 1;
    else if (_direction == 2)
        z -= 1;
    else if (_direction == 3)
        x -= 1;
    static int l = 2 * _boardLen;
    if (x <= -_boardLen + 1)
        x += (l + 1);
    if (x >= _boardLen)
        x -= (l + 1);
    if (z <= -_boardLen + 1)
        z += (l + 1);
    if (z >= _boardLen)
        z -= (l + 1);
}

void Triangle::Hide()
{
    MovePace(0, 0, 5);
}

void mBox::Hide()
{
    // std::cout << _x << ',' << _z << std::endl;
    checkBoard(_x, _z) = 0;
    MovePace(0, 0, 5);
}