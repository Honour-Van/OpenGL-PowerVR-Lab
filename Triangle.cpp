#include "Triangle.h"

Triangle::Triangle(void) : _vbo(0), _texture(0), _mvp(0),
    _position(glm::mat4(1.0f)), _rotation(glm::mat4(1.0f))
{
}

Triangle::~Triangle(void)
{
    // Release Vertex buffer object.
    if(_vbo) gl::DeleteBuffers(1, &_vbo);
    // Frees the texture
    if(_texture) gl::DeleteTextures(1, &_texture);
}

bool Triangle::Init(pvr::Shell* shell, uint32_t mvpLoc)
{
    GLfloat afVertices[] = { // Vertex 1
                            -0.4f, -0.4f, -0.0f,    // Position 1
                             2.0f, 0.0f, 0.0f,      // Texture coodinate 1
                             // Vertex 2
                             0.4f, -0.4f, -0.0f,    // Position 2
                             2.0f, 1.0f, 0.0f,      // Texture coodinate 2
                             // Vertex 3
                             0.0f,  0.4f, -0.0f,    // Position 3
                             2.0f, 0.5f, 1.0f,      // Texture coodinate 3
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

void Triangle::Update(float angle)
{
    // Rotate along Y Axis
    _rotation *= glm::rotate(angle, glm::vec3(0, -1, 0));
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

bool Cube::Init(pvr::Shell *shell, uint32_t mvpLoc)
{
    static char vertices[] = {
        -1,-1,-1, -1,-1, 1, -1, 1, 1,
        -1,-1,-1, -1, 1, 1, -1, 1,-1, // Left

         1, 1, 1, 1,-1,-1, 1, 1,-1,
         1,-1,-1, 1, 1, 1, 1,-1, 1, // Right

         1, 1,-1, -1,-1,-1, -1, 1,-1,
         1, 1,-1, 1,-1,-1, -1,-1,-1, // Back

        -1, 1, 1, -1,-1, 1, 1,-1, 1,
         1, 1, 1, -1, 1, 1, 1,-1, 1, // Front

         1, 1, 1, 1, 1,-1, -1, 1,-1,
         1, 1, 1, -1, 1,-1, -1, 1, 1, // Top

         1,-1, 1, -1,-1,-1, 1,-1,-1,
         1,-1, 1, -1,-1, 1, -1,-1,-1, // Bottom
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
    static const float CS = 0.03f; // Cube size

    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            vbodata[i * 36 + j * 6] = vertices[i * 18 + j * 3] * CS;
            vbodata[i * 36 + j * 6 + 1] = vertices[i * 18 + j * 3 + 1] * CS;
            vbodata[i * 36 + j * 6 + 2] = vertices[i * 18 + j * 3 + 2] * CS;
            int colori = i == 4 ? 4 : 6;
            for (int k = 0; k < 3; ++ k)
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
