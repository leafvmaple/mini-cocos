#include "platform/opengl/ZCOpenGLLoader.h"
#include <cstring>

#define LOAD(name)                                                                                 \
    do {                                                                                           \
        name = reinterpret_cast<decltype(name)>(getProc(#name));                                   \
        if (!name)                                                                                 \
            return false;                                                                          \
    } while (0)

void (*glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
void (*glViewport)(GLint, GLint, GLsizei, GLsizei);
void (*glClear)(GLbitfield);
void (*glEnable)(GLenum);
void (*glBlendFunc)(GLenum, GLenum);
void (*glGenVertexArrays)(GLsizei, GLuint*);
void (*glBindVertexArray)(GLuint);
void (*glGenBuffers)(GLsizei, GLuint*);
void (*glBindBuffer)(GLenum, GLuint);
void (*glBufferData)(GLenum, GLsizeiptr, const void*, GLenum);
void (*glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
void (*glEnableVertexAttribArray)(GLuint);
void (*glDrawArrays)(GLenum, GLint, GLsizei);
void (*glGenTextures)(GLsizei, GLuint*);
void (*glBindTexture)(GLenum, GLuint);
void (*glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
void (*glTexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*);
void (*glPixelStorei)(GLenum, GLint);
void (*glTexParameteri)(GLenum, GLenum, GLint);
void (*glActiveTexture)(GLenum);
GLuint (*glCreateShader)(GLenum);
void (*glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*);
void (*glCompileShader)(GLuint);
void (*glGetShaderiv)(GLuint, GLenum, GLint*);
void (*glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
GLuint (*glCreateProgram)(void);
void (*glAttachShader)(GLuint, GLuint);
void (*glLinkProgram)(GLuint);
void (*glGetProgramiv)(GLuint, GLenum, GLint*);
void (*glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
void (*glDeleteShader)(GLuint);
void (*glUseProgram)(GLuint);
GLint (*glGetUniformLocation)(GLuint, const GLchar*);
void (*glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
void (*glUniform1i)(GLint, GLint);
void (*glUniform1f)(GLint, GLfloat);
void (*glDeleteProgram)(GLuint);
void (*glDeleteTextures)(GLsizei, const GLuint*);
void (*glDeleteVertexArrays)(GLsizei, const GLuint*);
void (*glDeleteBuffers)(GLsizei, const GLuint*);

bool loadOpenGL(void* (*getProc)(const char*)) {
    LOAD(glClearColor);
    LOAD(glViewport);
    LOAD(glClear);
    LOAD(glEnable);
    LOAD(glBlendFunc);
    LOAD(glGenVertexArrays);
    LOAD(glBindVertexArray);
    LOAD(glGenBuffers);
    LOAD(glBindBuffer);
    LOAD(glBufferData);
    LOAD(glVertexAttribPointer);
    LOAD(glEnableVertexAttribArray);
    LOAD(glDrawArrays);
    LOAD(glGenTextures);
    LOAD(glBindTexture);
    LOAD(glTexImage2D);
    LOAD(glTexSubImage2D);
    LOAD(glPixelStorei);
    LOAD(glTexParameteri);
    LOAD(glActiveTexture);
    LOAD(glCreateShader);
    LOAD(glShaderSource);
    LOAD(glCompileShader);
    LOAD(glGetShaderiv);
    LOAD(glGetShaderInfoLog);
    LOAD(glCreateProgram);
    LOAD(glAttachShader);
    LOAD(glLinkProgram);
    LOAD(glGetProgramiv);
    LOAD(glGetProgramInfoLog);
    LOAD(glDeleteShader);
    LOAD(glUseProgram);
    LOAD(glGetUniformLocation);
    LOAD(glUniformMatrix4fv);
    LOAD(glUniform1i);
    LOAD(glUniform1f);
    LOAD(glDeleteProgram);
    LOAD(glDeleteTextures);
    LOAD(glDeleteVertexArrays);
    LOAD(glDeleteBuffers);
    return true;
}