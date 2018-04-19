#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "gl/glew.h"
#include "gl/freeglut.h"

using namespace std;

class CShader
{
public:
	GLuint prog;
	bool ok;

	CShader()
	{
		prog = 0;
		ok = false;
	}

	// loads chaders
	CShader(const char* vertexFilename, const char* fragmentFilename)
	{
		loadShader(vertexFilename, fragmentFilename);
	}

	GLuint getProgram()
	{
		return prog;
	}

	bool loadShader(const char* vertexFilename, const char* fragmentFilename)
	{
		if (prog)
		{
			// we should delete the current vao
		}
		ok = true;
		prog = 0;
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		// enabel exceptions
		vShaderFile.exceptions(std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::badbit);
		try
		{
			// opening the files
			vShaderFile.open(vertexFilename);
			fShaderFile.open(fragmentFilename);

			// reading files
			std::stringstream vShaderStream, fShaderStream;
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			vShaderFile.close();
			fShaderFile.close();

			// to string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e)
		{
			cout << "Error loading shader" << endl;
			ok = false;
			return false;
		}
		const char* vsCode = vertexCode.c_str();
		const char * fsCode = fragmentCode.c_str();
		GLuint vertex, fragment;
		GLint success;
		char infoLog[512];

		// vertex shaders... compiling....
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vsCode, NULL);
		glCompileShader(vertex);

		// check for compilation errors
		glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertex, 512, NULL, infoLog);
			cout << "Vertex shader error: " << infoLog << endl;
			ok = false;
			return false;
		}

		// fragment shaders... compiling....
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fsCode, NULL);
		glCompileShader(fragment);

		// check for compilation errors
		glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragment, 512, NULL, infoLog);
			std::cout << "Fragment shader error: " << infoLog << std::endl;
			ok = false;
			return false;
		}

		// Shader prog
		prog = glCreateProgram();
		glAttachShader(prog, vertex);
		glAttachShader(prog, fragment);
		glLinkProgram(prog);
		// Print linking errors if any
		glGetProgramiv(prog, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(prog, 512, NULL, infoLog);
			cout << "Shaders link error: " << infoLog << endl;
			ok = false;
			return false;
		}

		// shaders are linked into prog. We dont need them anymore
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		return true;
	}

	// set shader as current one
	void setCurrent()
	{
		if (prog)
			glUseProgram(prog);
		else
			cout << "CShader::setCurrent error: shader program does not exit" << endl;
	}
};

