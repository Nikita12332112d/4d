#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <GL/gl.h>
#include <cstddef>

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

// constants the 1.1 header does not carry
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif
#define GL_TEXTURE0 0x84C0
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_LINE_SMOOTH_HINT
#define GL_LINE_SMOOTH_HINT 0x0C52
#endif
#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif

typedef void(APIENTRY* PFN_glGenVertexArrays)(GLsizei, GLuint*);
typedef void(APIENTRY* PFN_glBindVertexArray)(GLuint);
typedef void(APIENTRY* PFN_glDeleteVertexArrays)(GLsizei, const GLuint*);
typedef void(APIENTRY* PFN_glGenBuffers)(GLsizei, GLuint*);
typedef void(APIENTRY* PFN_glBindBuffer)(GLenum, GLuint);
typedef void(APIENTRY* PFN_glBufferData)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void(APIENTRY* PFN_glBufferSubData)(GLenum, GLintptr, GLsizeiptr, const void*);
typedef void(APIENTRY* PFN_glDeleteBuffers)(GLsizei, const GLuint*);
typedef void(APIENTRY* PFN_glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei,
                                                  const void*);
typedef void(APIENTRY* PFN_glEnableVertexAttribArray)(GLuint);
typedef GLuint(APIENTRY* PFN_glCreateShader)(GLenum);
typedef void(APIENTRY* PFN_glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void(APIENTRY* PFN_glCompileShader)(GLuint);
typedef void(APIENTRY* PFN_glGetShaderiv)(GLuint, GLenum, GLint*);
typedef void(APIENTRY* PFN_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(APIENTRY* PFN_glDeleteShader)(GLuint);
typedef GLuint(APIENTRY* PFN_glCreateProgram)(void);
typedef void(APIENTRY* PFN_glAttachShader)(GLuint, GLuint);
typedef void(APIENTRY* PFN_glLinkProgram)(GLuint);
typedef void(APIENTRY* PFN_glGetProgramiv)(GLuint, GLenum, GLint*);
typedef void(APIENTRY* PFN_glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(APIENTRY* PFN_glUseProgram)(GLuint);
typedef void(APIENTRY* PFN_glDeleteProgram)(GLuint);
typedef GLint(APIENTRY* PFN_glGetUniformLocation)(GLuint, const GLchar*);
typedef void(APIENTRY* PFN_glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(APIENTRY* PFN_glUniform1f)(GLint, GLfloat);
typedef void(APIENTRY* PFN_glUniform1i)(GLint, GLint);
typedef void(APIENTRY* PFN_glUniform2f)(GLint, GLfloat, GLfloat);
typedef void(APIENTRY* PFN_glUniform3f)(GLint, GLfloat, GLfloat, GLfloat);
typedef void(APIENTRY* PFN_glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void(APIENTRY* PFN_glActiveTexture)(GLenum);

#define GL_FUNC_TABLE(X)                                    \
    X(PFN_glGenVertexArrays, glGenVertexArrays)             \
    X(PFN_glBindVertexArray, glBindVertexArray)             \
    X(PFN_glDeleteVertexArrays, glDeleteVertexArrays)       \
    X(PFN_glGenBuffers, glGenBuffers)                       \
    X(PFN_glBindBuffer, glBindBuffer)                       \
    X(PFN_glBufferData, glBufferData)                       \
    X(PFN_glBufferSubData, glBufferSubData)                 \
    X(PFN_glDeleteBuffers, glDeleteBuffers)                 \
    X(PFN_glVertexAttribPointer, glVertexAttribPointer)     \
    X(PFN_glEnableVertexAttribArray, glEnableVertexAttribArray) \
    X(PFN_glCreateShader, glCreateShader)                   \
    X(PFN_glShaderSource, glShaderSource)                   \
    X(PFN_glCompileShader, glCompileShader)                 \
    X(PFN_glGetShaderiv, glGetShaderiv)                     \
    X(PFN_glGetShaderInfoLog, glGetShaderInfoLog)           \
    X(PFN_glDeleteShader, glDeleteShader)                   \
    X(PFN_glCreateProgram, glCreateProgram)                 \
    X(PFN_glAttachShader, glAttachShader)                   \
    X(PFN_glLinkProgram, glLinkProgram)                     \
    X(PFN_glGetProgramiv, glGetProgramiv)                   \
    X(PFN_glGetProgramInfoLog, glGetProgramInfoLog)         \
    X(PFN_glUseProgram, glUseProgram)                       \
    X(PFN_glDeleteProgram, glDeleteProgram)                 \
    X(PFN_glGetUniformLocation, glGetUniformLocation)       \
    X(PFN_glUniformMatrix4fv, glUniformMatrix4fv)           \
    X(PFN_glUniform1f, glUniform1f)                         \
    X(PFN_glUniform1i, glUniform1i)                         \
    X(PFN_glUniform2f, glUniform2f)                         \
    X(PFN_glUniform3f, glUniform3f)                         \
    X(PFN_glUniform4f, glUniform4f)                         \
    X(PFN_glActiveTexture, glActiveTexture)

#define GL_DECLARE(type, name) extern type name;
GL_FUNC_TABLE(GL_DECLARE)
#undef GL_DECLARE

bool loadGlFunctions(const char** missing);
