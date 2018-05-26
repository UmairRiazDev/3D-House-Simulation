
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <math.h>

// SOIL to load textures
#include "SOIL/SOIL.h"

// GLM
#include "glm/glm.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/epsilon.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/compatibility.hpp"
#include "glm/ext.hpp"

// near clipping planes distance
#define NCP 1.0f

// far clipping planes distance
#define FCP 20000.0f

// how fast is the rotation per mouse "delta"
#define SPEED_ROTATE 0.005f

// how fast the user can be moved with the keyboard, in millimeters
#define SPEED_MOVE 100.0f

// objects folder relative to current folder
#define OBJPATH string("./objects/")

// number of objects in the scene
#define N_OBJECTS 23

//Walls color enum
#define RED 1
#define GREEN 2
#define BLUE 3


// libraries to link
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "soil32.lib")

//opengl 3.X & glsl 3XX availability
bool isOpenGL3Available = true;

// matrices
glm::mat4 g_model;
glm::mat4 g_normalMat;
glm::mat4 g_view;
glm::mat4 g_projection;

// rotation factor in x axis
float g_rx = 0.0f;

// camera
glm::vec3 g_position(0, 0, 0.f);
glm::vec3 g_front(0, 0, -1);
glm::vec3 g_up (0,1,0);
glm::vec3 g_right (1,0,0);

// last key pressed: 0=left, 1=right, 2=up, 3=down
int g_lastKeys[4];

// texture map: given a string, return its ID in OpenGL
map <string, unsigned int> g_texManager;

// shader 
CShader g_shader;

// window size
int   g_width = 1024;
int   g_height = 768;

// indicated if the left mouse click is pressed
bool g_leftPressed = false;

// last mouse position
int g_lastY, g_lastX;


// standar template library
using namespace std;

// useful data structure to store points or verteces
typedef struct SVertex
{
	float x, y, z;

	SVertex()
	{
		x = y = z = 0.0f;
	}

	SVertex(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	SVertex(const SVertex &a)
	{
		this->x = a.x;
		this->y = a.y;
		this->z = a.z;
	}
} SVertex;

// axis aligned bounding box structure
typedef struct SBox
{
	SVertex m_pMin, m_pMax;

	SBox()
	{

	}

	SBox(const SVertex &minimum, const SVertex &maximum)
	{
		m_pMin = minimum;
		m_pMax = maximum;
	}

	SBox &operator = (const SBox &x)
	{
		this->m_pMin = x.m_pMin;
		this->m_pMax = x.m_pMax;
		return *this;
	}
} SBox;

// test collision between a box and a set opf boxes
class CollisionMap : public vector<SBox>
{
public:
	CollisionMap()
	{}

	void addBox(const SBox &x)
	{
		push_back(x);
	}

	bool inter1D(float a, float b, float c, float d)
	{
		return (b <= c || d <= a) ? false : true;
	}

	bool collide(const SBox &b)
	{
		int n = size();
		for (int i = 0; i < n; i++)
		{
			const SBox &s = (*this)[i];
			// we reduce the intersection of two boxes by the intersection of 3 intervals
			if (inter1D(s.m_pMin.x, s.m_pMax.x, b.m_pMin.x, b.m_pMax.x) &&
				inter1D(s.m_pMin.y, s.m_pMax.y, b.m_pMin.y, b.m_pMax.y) &&
				inter1D(s.m_pMin.z, s.m_pMax.z, b.m_pMin.z, b.m_pMax.z))
				return true;
		}
		return false;
	}
};

// useful data structure to store angle and axis to rotate
typedef struct Quaternion
{
	float x, y, z, w;

	Quaternion()
	{
		x = y = z = w = 0.0f;
	}

	Quaternion(float x, float y, float z, float w = 0.0f)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	Quaternion(const Quaternion &a)
	{
		this->x = a.x;
		this->y = a.y;
		this->z = a.z;
		this->w = a.w;
	}
} SVertex4;

// useful data structure to store texture coordinates
typedef struct STexCoord
{
	float s, t;

	STexCoord()
	{

	}

	STexCoord(float s, float t)
	{
		this->s = s;
		this->t = t;
	}

	STexCoord(const STexCoord &a)
	{
		this->s = a.s;
		this->t = a.t;
	}
} STexCoord;



// shader stuff: location of each shader item
GLuint iLocPosition;
GLuint iLocNormal;
GLuint iLocTexCoord;
GLuint iLocModel;
GLuint iLocNormalMat;
GLuint iLocView;
GLuint iLocProjection;
GLuint iLocKa;
GLuint iLocKd;
GLuint iLocKs;
GLuint iLocShine;
GLuint iLocTexture_diffuse1;
GLuint iLocHasTexture;
GLuint iLocHasNormal;


// a material
typedef struct SMaterial
{
	string m_name;
	SVertex m_ambient;
	SVertex m_diffuse;
	SVertex m_specular;
	float   m_shininess;
	string  m_diffuseFileName;

	SMaterial()
	{
		m_name = string("");
		m_shininess = 0;
	}


	SMaterial(const string &name)
	{
		m_name = name;
		m_shininess = 0;
	}

	// given a texture path, return the opengl ID... 
	// of the texture has not been loaded, it loads it just 1 time
	unsigned int getTexture(const string &path)
	{
		if (g_texManager.find(path) == g_texManager.end())
		{
			unsigned int ret = SOIL_load_OGL_texture((OBJPATH + path).c_str(),
				SOIL_LOAD_AUTO,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_MIPMAPS | SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_DDS_LOAD_DIRECT);
			if (ret)
			{
				g_texManager[path] = ret;
				return ret;
			}
			else
			{
				printf("Error loading %s\nPress enter too finish-->", path.c_str());
				getchar();
				exit(1);
				return 0;
			}

		}
		return g_texManager[path];
	}

	// set all values into a program shader
	void set(GLuint p)
	{
		// material properties
		glUniform4f(iLocKa, m_ambient.x, m_ambient.y, m_ambient.z, 1.0f);
		glUniform4f(iLocKd, m_diffuse.x, m_diffuse.y, m_diffuse.z, 1.0f);
		glUniform4f(iLocKs, m_specular.x, m_specular.y, m_specular.z, 1.0f);
		glUniform1f(iLocShine, m_shininess / 4.0);

		// we check if the material includes a diuffuse map or notr
		if (m_diffuseFileName.compare(""))
		{
			GLuint t = getTexture(m_diffuseFileName);
			if (t != 0)
			{
				// setting the texture ID
				glUniform1i(iLocTexture_diffuse1, 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, t);
				glUniform1i(iLocHasTexture, 1);
			}
			else
				glUniform1i(iLocHasTexture, 0);
		}
		else
			glUniform1i(iLocHasTexture, 0);

	}
} SMaterial;

// one mesh of the object
typedef struct SMesh
{
	vector<SVertex> m_verteces;
	vector<SVertex> m_normals;
	vector<STexCoord> m_texCoords;

	// bounding box
	SVertex m_min, m_max;

	GLuint m_vao, m_v, m_t, m_n;
	int m_materialIndex;

	SMesh()
	{
		m_materialIndex = -1; 
		m_vao = 0;
	}

	SMesh(int matIndex)
	{
		m_materialIndex = matIndex;
		m_vao = 0;
	}

	SMesh & operator = (const SMesh &m)
	{
		this->m_verteces = m.m_verteces;
		this->m_normals = m.m_normals;
		this->m_texCoords = m.m_texCoords;
		this->m_min = m.m_min;
		this->m_max = m.m_max;
		this->m_materialIndex = m.m_materialIndex;
		this->m_vao = m.m_vao;

	}

	// load the mesh into the GPU, if it has not been loaded before
	void loadIntoGPU(GLuint p)
	{
		if (m_vao)
			return;
		// create vao
		if (isOpenGL3Available) {
			glGenVertexArrays(1, &m_vao);
		}
		else {
			glGenBuffers(1, &m_vao);
		}
		glGenBuffers(1, &m_v);
		glGenBuffers(1, &m_n);
		glGenBuffers(1, &m_t);

		// setting vao as current
		GLint pos0, pos1, pos2;
		if (isOpenGL3Available) {
			glBindVertexArray(m_vao);
			//for GLSL 3xx: positions are defined in shaders itself.
			pos0 = 0;
			pos1 = 1;
			pos2 = 2;
		}
		else {
			//for GLSL 1xx: positions depend on runtime.
			glBindBuffer(GL_ARRAY_BUFFER, m_vao);
			pos0 = glGetAttribLocation(p, "inPosition");
			pos1 = glGetAttribLocation(p, "inNormal");
			pos2 = glGetAttribLocation(p, "inTex");
		}
		
		if (m_verteces.size())
		{
			// uploading vertexes
			glBindBuffer(GL_ARRAY_BUFFER, m_v);
			glEnableVertexAttribArray(pos0);
			glBufferData(GL_ARRAY_BUFFER, m_verteces.size() * sizeof(SVertex), m_verteces.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(pos0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}
		if (m_normals.size())
		{
			// upload normals
			glBindBuffer(GL_ARRAY_BUFFER, m_n);
			glEnableVertexAttribArray(pos1);
			glBufferData(GL_ARRAY_BUFFER, m_normals.size() * sizeof(SVertex), m_normals.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(pos1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}
		if (m_texCoords.size())
		{
			// upload texture coordinates
			glBindBuffer(GL_ARRAY_BUFFER, m_t);
			glEnableVertexAttribArray(pos2);
			glBufferData(GL_ARRAY_BUFFER, m_texCoords.size() * sizeof(STexCoord), m_texCoords.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(pos2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		}

		// unbinding
		if (isOpenGL3Available)
			glBindVertexArray(NULL);
	}

	// render the mesh using a program p and a material mat
	void render(GLuint p, SMaterial &mat)
	{
		loadIntoGPU(p);
		mat.set(p);
		glUniform1i(iLocHasNormal,  m_normals.size() ? 1 : 0);
		GLint pos0, pos1, pos2;
		if (isOpenGL3Available)
		{
			glBindVertexArray(m_vao);
			pos0 = 0;
			pos1 = 1;
			pos2 = 2;
		}
		else {
			glBindBuffer(GL_ARRAY_BUFFER, m_vao);
			pos0 = glGetAttribLocation(p, "inPosition");
			pos1 = glGetAttribLocation(p, "inNormal");
			pos2 = glGetAttribLocation(p, "inTex");
		}

		// pos
		glBindBuffer(GL_ARRAY_BUFFER, m_v);
		glEnableVertexAttribArray(pos0);
		glVertexAttribPointer(pos0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		// normal
		if (m_normals.size())
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_n);
			glEnableVertexAttribArray(pos1);
			glVertexAttribPointer(pos1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}
		else
			glDisableVertexAttribArray(pos1);
		// tex
		if (m_texCoords.size())
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_t);
			glEnableVertexAttribArray(pos2);
			glVertexAttribPointer(pos2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		}
		else
			glDisableVertexAttribArray(pos2);
		// render here
		glDrawArrays(GL_TRIANGLES, 0, m_verteces.size());

		glDisableVertexAttribArray(pos0);
		glDisableVertexAttribArray(pos1);
		glDisableVertexAttribArray(pos2);
		if (isOpenGL3Available)
			glBindVertexArray(0);

	}
} SMesh;

// the mesh object
class C3DObject
{

public:
	C3DObject()
	{
		m_size = NULL;
		m_position = NULL;
		m_rotation = NULL;
		m_euler = NULL;
	}

	~C3DObject()
	{
		if (m_size)
			delete m_size;
		if (m_position)
			delete m_position;
		if (m_rotation)
			delete m_rotation;
		if (m_euler)
			delete m_euler;
	}

	// load materla filw
	int loadMTL(const char *matName)
	{
		//m_materials.clear();
		FILE *f = fopen((OBJPATH + matName).c_str(), "rt");
		if (f == NULL)
		{
			printf("File %d not found\n", matName);
			return 1;
		}

		int error_code = 0;
		char _line[1001];
		char *line;

		// loading lines
		int n = 0, index = -1;
		while (!error_code && fgets(_line, 1000, f))
		{
			int len = strlen(_line);

			// removing enter and white spaces at the end
			while (len > 0 && _line[len - 1] <= ' ')
				_line[--len] = 0x00;
			if (len <= 0) continue;
			line = _line;
			while (len > 0  && *line <= ' ')
			{
				line++;
				len--;
			}
			if (len <= 0) continue;


			// a material found
			if (strstr(line, "newmtl"))
			{
				char *p = strstr(line, "newmtl");
				p += strlen("newmtl");
				while (*p <= ' ') p++;
				index = getMaterialIndex(string(p));
			}
			else if (line[0] == 'K' && line[1] == 'a'&& line[2] == ' ')
			{
				// ambient constant
				if (index != -1)
				{
					float r, g, b;
					int ret = sscanf(line, "Ka %f %f %f", &r, &g, &b);
					if (ret == 3)
						m_materials[index].m_ambient = SVertex(r, g, b);
					else
						error_code = 2;

				}
				else
					error_code = 100;
			}
			else if (line[0] == 'K' && line[1] == 'd'&& line[2] == ' ')
			{
				// diffuse constant
				if (index != -1)
				{
					float r, g, b;
					int ret = sscanf(line, "Kd %f %f %f", &r, &g, &b);
					if (ret == 3)
						m_materials[index].m_diffuse = SVertex(r, g, b);
					else
						error_code = 2;
				}
				else
					error_code = 100;
			}
			else if (line[0] == 'K' && line[1] == 's' && line[2] == ' ')
			{
				// specular
				if (index != -1)
				{
					float r, g, b;
					int ret = sscanf(line, "Ks %f %f %f", &r, &g, &b);
					if (ret == 3)
						m_materials[index].m_specular = SVertex(r, g, b);
					else
						error_code = 2;
				}
				else
					error_code = 100;
			}
			else if (line[0] == 'N' && line[1] == 's' && line[2] == ' ')
			{
				// shine
				if (index != -1)
				{
					float s;
					int ret = sscanf(line, "Ns %f", &s);
					if (ret == 1)
						m_materials[index].m_shininess = s;
					else
						error_code = 2;
				}
				else
					error_code = 100;
			}
			else if (strstr(line, "map_Kd") == line)
			{
				// diffuse map
				if (index != -1)
				{
					char *p = line;
					p += strlen("map_Kd");
					while (*p <= ' ') p++;
					m_materials[index].m_diffuseFileName = string(p);
				}
				else
					error_code = 100;
			}
		}
		fclose(f);
		return 0;
	}

	// get the material index, given the material name
	// if the material index does not exit, it creates a new entry 
	int getMaterialIndex(const string &matName)
	{
		for (int i = 0; i<m_materials.size(); i++)
		{
			if (m_materials[i].m_name.compare(matName)==0)
				return i;
		}

		int index = m_materials.size();
		m_materials.push_back(SMaterial(matName));
		m_meshes.push_back(SMesh(index));
		return index;
	}
	
	// split the like using a separator
	void splitLine(const char *line, vector<string> &words, int c = ' ')
	{
		words.clear();
		int i = 0;
		int n = strlen(line);
		while (i < n)
		{
			string w;
			while (line[i] == c) i++;
			if (i == n) break;
			while (i < n && line[i] != c)
			{
				w.push_back(line[i]);
				i++;
			}
			words.push_back(w);
		}
	}

	// split the line into integer values, using a separator
	void splitLine(const char *line, vector<int> &values, int c = '/')
	{
		values.clear();
		int i = 0;
		int n = strlen(line);
		while (i < n)
		{
			string w;
			while (line[i] == ' ') i++;
			if (line[i] == c) i++;
			while (line[i] == ' ') i++;
			if (i == n) break;
			while (i < n && line[i] != c)
			{
				w.push_back(line[i]);
				i++;
			}
			if (w.size() == 0)
				values.push_back(-10000000);
			else
				values.push_back(atoi(w.c_str()));
		}
	}

	// load obj file
	int loadOBJ(const char *filename)
	{
		m_meshes.clear();
		m_materials.clear();
		FILE *f = fopen((OBJPATH + filename).c_str(), "rt");

		int error_code = 0;
		if (f == NULL) 
		{
			printf("File %d not found\n", filename);
			return 1;
		}

		char _line[1001];
		char *line;

		vector<SVertex> packet_verteces;
		vector<SVertex> packet_normals;
		vector<STexCoord> packet_texCoords;

		// loading lines
		int n = 0, index = -1;
		while (!error_code && fgets(_line, 1000, f))
		{
			int len = strlen(_line);

			// removing enter and white spaces at the end
			while (len > 0 && _line[len - 1] <= ' ')
				_line[--len] = 0x00;
			if (len <= 0) continue;
			line = _line;
			while (len > 0  && *line <= ' ')
			{
				line++;
				len--;
			}
			if (len <= 0) continue;

			// a new material found?
			if (strstr(line, "usemtl"))
			{
				char *p = strstr(line, "usemtl");
				p += strlen("usemtl");
				while (*p <= ' ') p++;

				index = getMaterialIndex(string(p));
			}
			else if (line[0] == 'v' && line[1] == ' ')
			{
				float x, y, z;
				int ret = sscanf(line, "v %f %f %f", &x, &y, &z);
				if (ret == 3)
					packet_verteces.push_back(SVertex(x, y, z));
				else
					error_code = 2;
			}
			else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
			{
				float x, y, z;
				if (sscanf(line, "vn %f %f %f", &x, &y, &z) == 3)
					packet_normals.push_back(SVertex(x, y, z));
				else
					error_code = 3;
			}
			else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ')
			{
				float s, t;
				if (sscanf(line, "vt %f %f", &s, &t) == 2)
					packet_texCoords.push_back(STexCoord(s, t));
				else
					error_code = 4;
			}
			else if (line[0] == 'f' && line[1] == ' ')
			{
				// face
				vector<string> words;
				splitLine(line+1, words);
				if (words.size() <= 2)	// we do not support lines or points in this implementation
					continue;
				vector <int> sizes;
				sizes.push_back(packet_verteces.size());
				sizes.push_back(packet_texCoords.size());
				sizes.push_back(packet_normals.size());
				vector<int> e0, e1, e2;

				splitLine(words[0].c_str(), e0); myAbs(e0, sizes);
				splitLine(words[1].c_str(), e1); myAbs(e1, sizes);

				for (int j = 0; j + 2 < words.size(); j++, e1 = e2)
				{
					sizes.clear();
					sizes.push_back(packet_verteces.size());
					sizes.push_back(packet_texCoords.size());
					sizes.push_back(packet_normals.size());
					splitLine(words[j+2].c_str(), e2); myAbs(e2, sizes);

					m_meshes[index].m_verteces.push_back(packet_verteces[e0[0] - 1]);
					m_meshes[index].m_verteces.push_back(packet_verteces[e1[0] - 1]);
					m_meshes[index].m_verteces.push_back(packet_verteces[e2[0] - 1]);

					if (e1.size() > 1 && packet_texCoords.size() > 0)
					{
						// because, we may have an entry like this A//C
						if (e0[1] != -10000000)
							m_meshes[index].m_texCoords.push_back(packet_texCoords[e0[1] - 1]);
						if (e1[1] != -10000000)
							m_meshes[index].m_texCoords.push_back(packet_texCoords[e1[1] - 1]);
						if (e2[1] != -10000000)
							m_meshes[index].m_texCoords.push_back(packet_texCoords[e2[1] - 1]);
					}
					if (e1.size() > 2)
					{
						if (e0.size() > 2)
							m_meshes[index].m_normals.push_back(packet_normals[e0[2] - 1]);
						else
							m_meshes[index].m_normals.push_back(packet_normals[e1[2] - 1]);

						m_meshes[index].m_normals.push_back(packet_normals[e1[2] - 1]);

						if (e2.size() > 2)
							m_meshes[index].m_normals.push_back(packet_normals[e2[2] - 1]);
						else
							m_meshes[index].m_normals.push_back(packet_normals[e1[2] - 1]);
					}
				}
			}
		}
		fclose(f);
		if (error_code)
			return error_code;

		for (int k = 0; k < m_materials.size(); k++)
		{
			const vector<SVertex> &v = m_meshes[k].m_verteces;

			m_meshes[k].m_min = m_meshes[k].m_max = v[0];

			// update bbbox per mesh
			for (int i = 0; i < v.size(); i++)
			{
				if (v[i].x < m_meshes[k].m_min.x)  m_meshes[k].m_min.x = v[i].x;
				if (v[i].y < m_meshes[k].m_min.y)  m_meshes[k].m_min.y = v[i].y;
				if (v[i].z < m_meshes[k].m_min.z)  m_meshes[k].m_min.z = v[i].z;
				if (v[i].x > m_meshes[k].m_max.x)  m_meshes[k].m_max.x = v[i].x;
				if (v[i].y > m_meshes[k].m_max.y)  m_meshes[k].m_max.y = v[i].y;
				if (v[i].z > m_meshes[k].m_max.z)  m_meshes[k].m_max.z = v[i].z;
			}

			// update global bbox 
			if (k == 0)
			{
				m_min = m_meshes[k].m_min;
				m_max = m_meshes[k].m_max;
			}
			else
			{
				if (m_meshes[k].m_min.x < m_min.x)  m_min.x = m_meshes[k].m_min.x;
				if (m_meshes[k].m_min.y < m_min.y)  m_min.y = m_meshes[k].m_min.y;
				if (m_meshes[k].m_min.z < m_min.z)  m_min.z = m_meshes[k].m_min.z;

				if (m_meshes[k].m_max.x > m_max.x)  m_max.x = m_meshes[k].m_max.x;
				if (m_meshes[k].m_max.y > m_max.y)  m_max.y = m_meshes[k].m_max.y;
				if (m_meshes[k].m_max.z > m_max.z)  m_max.z = m_meshes[k].m_max.z;
			}
		}
		return 0;
	}

	// transform indeces to positive indexes
	void myAbs(vector<int> &a, vector<int> &n)
	{
		for (int i = 0; i<a.size(); i++)
			if (a[i] < 0 && a[i] != -10000000)
				a[i] = n[i] + a[i] + 1;
	}

	// set the object bounding box (location in 0-space)
	void worldBoundingBox(float x0, float y0, float z0, float x1, float y1, float z1)
	{
		if (!m_size)
			m_size = new SVertex;
		m_size->x = fabs(x1 - x0);
		m_size->y = fabs(y1 - y0);
		m_size->z = fabs(z1 - z0);
		if (!m_position)
			m_position = new SVertex;
		m_position->x = (x1 + x0) * 0.5f;
		m_position->y = (y1 + y0) * 0.5f;
		m_position->z = (z1 + z0) * 0.5f;
	}

	// set the nobject position in world space
	void worldLocation(float cx, float cy, float cz)
	{
		if (!m_position)
			m_position = new SVertex;
		m_position->x = cx;
		m_position->y = cy;
		m_position->z = cz;
	}

	// set the object scaling
	void scaleObject(float sx, float sy, float sz)
	{
		if (!m_size)
			m_size = new SVertex;
		m_size->x = sx;
		m_size->y = sy;
		m_size->z = sz;

	}

	// set the rotation angles of the object
	void setEuler(float x, float y, float z)
	{
		if (!m_euler)
			m_euler = new SVertex;
		m_euler->x = x;
		m_euler->y = y;
		m_euler->z = z;
	}

	// set the rotation angle around a given vector
	void setRotation(float angle, float x, float y, float z)
	{
		if (!m_rotation)
			m_rotation = new Quaternion;
		m_rotation->x = x;
		m_rotation->y = y;
		m_rotation->z = z;
		m_rotation->w = angle;
	}

	// render the object using a shader program p
	void render(GLuint p)
	{
		g_view = glm::rotate(g_rx, glm::vec3(1.0f, 0.0f, 0.0f) ) * glm::lookAt(g_position, g_position + g_front, g_up);
		glUniformMatrix4fv(iLocView, 1, GL_FALSE, glm::value_ptr(g_view));

		SVertex center((m_min.x + m_max.x) * 0.5f, (m_min.y + m_max.y) * 0.5f, (m_min.z + m_max.z) * 0.5f);
		SVertex lengths(m_max.x - m_min.x, m_max.y - m_min.y, m_max.z - m_min.z);
		float maxLength = lengths.x;
		if (lengths.y > maxLength) maxLength = lengths.y;
		if (lengths.z > maxLength) maxLength = lengths.z;
		//static float angle = 0.0f;
		//angle += 0.001f;
		//if (angle > 360.0f) angle -= 360.0f;
		g_model = glm::mat4(1.0f);
		if (m_position)
			g_model *= glm::translate(glm::vec3(m_position->x, m_position->y, m_position->z));
			//glm::translate(glm::vec3(0,0,-1.0f)) *
			//glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f)) *
			//glm::rotate(angle*0.5f, glm::vec3(1.0f, 0.0f, 0.0f)) *
			//(m_size ?	glm::scale(glm::vec3(m_size->x / maxLength, m_size->y / maxLength, m_size->z / maxLength)) : glm::scale(glm::vec3(1.0f/maxLength, 1.0f / maxLength, 1.0f / maxLength))) *
		if (m_rotation)
		{
			g_model *= glm::rotate(m_rotation->w, glm::vec3(m_rotation->x, m_rotation->y, m_rotation->z));
			g_normalMat = glm::rotate(m_rotation->w, glm::vec3(m_rotation->x, m_rotation->y, m_rotation->z));
		}
		else
			g_normalMat = glm::mat4(1.0f);

		if (m_euler)
		{
			g_model *= glm::rotate(m_euler->z, glm::vec3(0, 0, 1));
			g_model *= glm::rotate(m_euler->y, glm::vec3(0, 1, 0));
			g_model *= glm::rotate(m_euler->x, glm::vec3(1, 0, 0));

			g_normalMat *= glm::rotate(m_euler->z, glm::vec3(0, 0, 1));
			g_normalMat *= glm::rotate(m_euler->y, glm::vec3(0, 1, 0));
			g_normalMat *= glm::rotate(m_euler->x, glm::vec3(1, 0, 0));
		}

		if (m_size)
			g_model *= glm::scale(glm::vec3(m_size->x / lengths.x, m_size->y / lengths.y, m_size->z / lengths.z));
		else
			g_model *= glm::scale(glm::vec3(1.0f / maxLength, 1.0f / maxLength, 1.0f / maxLength));
		g_model *= glm::translate(glm::vec3(-center.x, -center.y, -center.z));
		glUniformMatrix4fv(iLocModel, 1, GL_FALSE, glm::value_ptr(g_model));
		glUniformMatrix4fv(iLocNormalMat, 1, GL_FALSE, glm::value_ptr(g_normalMat));

		for (int i = 0; i < m_meshes.size(); i++) if (m_meshes[i].m_verteces.size() > 0)
		{
			m_meshes[i].render(g_shader.getProgram(), m_materials[m_meshes[i].m_materialIndex]);
		}
	}

	// bounding box in object space
	SVertex m_min, m_max;

	// array of meshes
	vector<SMesh> m_meshes;

	// array of materials
	vector<SMaterial> m_materials;

	// scaling factors
	SVertex *m_size;

	// position in eye space
	SVertex *m_position;

	// rotation angles
	SVertex *m_euler;
	Quaternion *m_rotation;
};


C3DObject g_obj[N_OBJECTS];
CollisionMap g_collMap;

// keyboard callback
void keyboardDown(unsigned char k, int x, int y) 
{
	switch(k)  
	{
		case 'q': case  27: exit(0);
	}
}

//update the camera according to the pressed keys
void updateCamera()
{
	if (g_lastKeys[0])
		g_front = glm::rotate(4 * SPEED_ROTATE, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(g_front, 0.0f);

	if (g_lastKeys[1])
		g_front = glm::rotate(-4 * SPEED_ROTATE, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(g_front, 0.0f);

	const float userWidth = 200.0f;
	const float userHeight = 2000.0f;
	if (g_lastKeys[2])
	{
		SBox b;
		glm::vec3 newPos = g_position + SPEED_MOVE* (glm::vec3(g_front.x, 0.0f, g_front.z));
		b.m_pMin.x = newPos.x - userWidth / 2.0f;
		b.m_pMax.x = newPos.x + userWidth / 2.0f;
		b.m_pMin.y = newPos.y - userHeight / 2.0f;
		b.m_pMax.y = newPos.y + userHeight / 2.0f;
		b.m_pMin.z = newPos.z - userWidth / 2.0f;
		b.m_pMax.z = newPos.z + userWidth / 2.0f;
		if (!g_collMap.collide(b))
			g_position = newPos;
	}

	if (g_lastKeys[3])
	{
		SBox b;
		glm::vec3 newPos = g_position - SPEED_MOVE* (glm::vec3(g_front.x, 0.0f, g_front.z));
		b.m_pMin.x = newPos.x - userWidth / 2.0f;
		b.m_pMax.x = newPos.x + userWidth / 2.0f;
		b.m_pMin.y = newPos.y - userHeight / 2.0f;
		b.m_pMax.y = newPos.y + userHeight / 2.0f;
		b.m_pMin.z = newPos.z - userWidth / 2.0f;
		b.m_pMax.z = newPos.z + userWidth / 2.0f;
		if (!g_collMap.collide(b))
			g_position = newPos;
	}
}

// draw callback
void drawCallback() 
{
	glClearColor(0.4, 0.5, 0.8, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	updateCamera();
	for (int i = 0; i < N_OBJECTS; i++)
		g_obj[i].render(g_shader.getProgram());
	
	glutSwapBuffers();
	Sleep(1000 / 60);
}

//resize callback
void reshapeCallback(int w, int h) 
{
	if (h == 0)
		return;
	glViewport(0, 0, w, h);
	g_width  = w;   
	g_height = h;   
	g_projection = glm::perspective(3.14159f / 3.0f, (float)g_width / (float)g_height, NCP, FCP);
	glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, glm::value_ptr(g_projection));
}

// opengl initialization
void initOpengl() 
{
	memset(g_lastKeys, 0, sizeof(int) * 4);
	glEnable(GL_DEPTH_TEST);
	
	// loading vertex and fragment shaders
	try 
	{
		 char* vertex_shader;
		 char* fragment_shader;
		 if (isOpenGL3Available) {
			 vertex_shader = "vertex.shader";
			 fragment_shader = "vertex.shader";
		 }
		 else {
			 vertex_shader = "vertex120.shader";
			 fragment_shader = "fragment120.shader";
		 }
		
		 if (g_shader.loadShader(vertex_shader, fragment_shader) == false)
		 {
			 printf("Error in shaders. Press enter to finish-->\n");
			 getchar();
			 exit(1);
		 }
	 }
	catch(exception) {
		printf("Exception in shaders\n");;
	}

	g_shader.setCurrent();
	
	// initializing location of each shader
	iLocPosition = glGetAttribLocation(g_shader.getProgram(), "inPosition");
	iLocNormal   = glGetAttribLocation(g_shader.getProgram(), "inNormal");
	iLocTexCoord = glGetAttribLocation(g_shader.getProgram(), "inTex");
	iLocModel      = glGetUniformLocation(g_shader.getProgram(), "model");
	iLocNormalMat  = glGetUniformLocation(g_shader.getProgram(), "normalMat");
	iLocView       = glGetUniformLocation(g_shader.getProgram(), "view");
	iLocProjection = glGetUniformLocation(g_shader.getProgram(), "projection");
	iLocKa = glGetUniformLocation(g_shader.getProgram(), "ka");
	iLocKd = glGetUniformLocation(g_shader.getProgram(), "kd");
	iLocKs = glGetUniformLocation(g_shader.getProgram(), "ks");
	iLocShine = glGetUniformLocation(g_shader.getProgram(), "shine");
	iLocTexture_diffuse1 = glGetUniformLocation(g_shader.getProgram(), "texture_diffuse1");
	iLocHasTexture = glGetUniformLocation(g_shader.getProgram(), "hasTexture");
	iLocHasNormal = glGetUniformLocation(g_shader.getProgram(), "hasNormal");
	
	// default projection  nmatrix
	g_projection = glm::perspective(3.14159f / 3.0f, (float)g_width / (float)g_height, NCP, FCP);
	glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, glm::value_ptr(g_projection));
}

// usefull functionm to load object material files
void loadObjMat(C3DObject & obj, const char *objFilename, const char *matFilename)
{
	int ret = obj.loadOBJ(objFilename);
	if (ret)
	{
		printf("Error %d reading %s\nPress enter-->", ret, objFilename);
		getchar();
		exit(1);
	}
	else
	{
		printf("%s have been read\n", objFilename);
		ret = obj.loadMTL(matFilename);
		if (ret)
		{
			printf("Error %d reading %s\nPress enter-->", ret, matFilename);
			getchar();
			exit(1);
		}
		else
			printf("%s have been read\n", matFilename);
	}
}

// special key callback (used for the arrow keys)
void specialKeyboardDown(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		g_lastKeys[0] = 1;
		break;

	case GLUT_KEY_RIGHT:
		g_lastKeys[1] = 1;
		break;
		break;

	case GLUT_KEY_UP:
		g_lastKeys[2] = 1;
		break;

	case GLUT_KEY_DOWN:
		g_lastKeys[3] = 1;
		break;
	}
}

// releasing a key
void specialKeyboardUp(int key, int x, int y) 
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		g_lastKeys[0] = 0;
		break;

	case GLUT_KEY_RIGHT:
		g_lastKeys[1] = 0;
		break;
		break;

	case GLUT_KEY_UP:
		g_lastKeys[2] = 0;
		break;

	case GLUT_KEY_DOWN:
		g_lastKeys[3] = 0;
		break;
	}
}

// called when left mouse has been clicked
void MouseLeftClick(int button, int state, int x, int y)
{
	if (GLUT_LEFT_BUTTON == button)
		g_leftPressed = (state == GLUT_DOWN);
	g_lastX = x;
	g_lastY = g_height - 1 - y;
}

// called when mouse has moved
void MouseMove(int x, int y)
{
	if (g_leftPressed)
	{
		int vx = g_lastX - x;
		int vy = g_height - 1 - y - g_lastY;

		// computing rotation in Y axis
		g_front = glm::rotate(vx*0.01f, glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f))) * glm::vec4(g_front, 0.0f);
		g_front = glm::normalize(g_front);

		// computing rotation in x axis
		g_rx -= vy*0.01f;
		if (g_rx > 1.0f)
			g_rx = 1.0f;
		else if (g_rx < -1.0f)
			g_rx = -1.0f;
	}
	g_lastX = x;
	g_lastY = g_height - 1 - y;
}

void processMenuEvents(int option) {

	switch (option) {
	case RED:
		loadObjMat(g_obj[0], "house.obj", "house-redwalls.mtl");
		break;
	case GREEN:
		loadObjMat(g_obj[0], "house.obj", "house-greenwalls.mtl"); 
		break;
	case BLUE:
		loadObjMat(g_obj[0], "house.obj", "house-bluewalls.mtl");
		break;
	}
}

int main(int argc, char** argv) 
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(1024, 768);

	int menu = glutCreateMenu(processMenuEvents);
	//add entries to our menu
	glutAddMenuEntry("Red Walls", RED);
	glutAddMenuEntry("Blue Walls", BLUE);
	glutAddMenuEntry("Green Walls", GREEN);

	// attach the menu to the right button
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	char* ver = (char*)glGetString(GL_VERSION);
	isOpenGL3Available = ver[0] > '2';
	if (!isOpenGL3Available)
		glewExperimental = GLU_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		return 0;
	}
	glutCreateWindow("3D Virtual House");
	glewInit();
	initOpengl();

	// loading all .obj and .mat
	loadObjMat(g_obj[0], "house.obj", "house.mtl");
	loadObjMat(g_obj[1], "3dstylish-fbde01.obj", "3dstylish-fbde01.mtl");
	loadObjMat(g_obj[2], "desk.obj", "desk.mtl");
	loadObjMat(g_obj[3], "Samsung LED TV.obj", "Samsung LED TV.mtl");
	loadObjMat(g_obj[4], "bed.obj", "bed.mtl");
	loadObjMat(g_obj[5], "Wardrobe_modular_system_final.obj", "Wardrobe_modular_system_final.mtl");
	loadObjMat(g_obj[6], "3d-model.obj", "3d-model.mtl");
	loadObjMat(g_obj[7], "table.obj", "table.mtl");
	loadObjMat(g_obj[8], "toilet3.obj", "toilet3.mtl");
	loadObjMat(g_obj[9], "bidet.obj", "bidet.mtl");
	loadObjMat(g_obj[10], "shower-door.obj", "shower-door.mtl");
	loadObjMat(g_obj[11], "shower-door.obj", "shower-door.mtl");
	loadObjMat(g_obj[12], "toilet3.obj", "toilet3.mtl");
	loadObjMat(g_obj[13], "bidet.obj", "bidet.mtl");
	loadObjMat(g_obj[14], "cucina.obj", "cucina.mtl");
	loadObjMat(g_obj[15], "sink-oldstyle.obj", "sink-oldstyle.mtl");
	loadObjMat(g_obj[16], "refrigerator.obj", "refrigerator.mtl");
	loadObjMat(g_obj[17], "sofa.obj", "sofa.mtl");
	loadObjMat(g_obj[18], "table.obj", "table.mtl");
	loadObjMat(g_obj[19], "Samsung LED TV.obj", "Samsung LED TV.mtl");
	loadObjMat(g_obj[20], "VenetianBlind.obj", "VenetianBlind.mtl");
	loadObjMat(g_obj[21], "VenetianBlind.obj", "VenetianBlind.mtl");
	loadObjMat(g_obj[22], "Window_001 no glass.obj", "Window_001.mtl");
	
	// setting the location, size, rotation of every object into the scene
	g_obj[0].worldBoundingBox(0, 0, -8385, 12442, 2500, 0);
	g_obj[1].worldBoundingBox(11009 - 240 - 2000, 0, -675 - 240 - 2000, 11009 - 240,      700, -675 - 240);
	g_obj[1].setRotation(3.14159f, 0, 1, 0);
	g_obj[2].worldBoundingBox(11009 - 240 - 2000, 0, -775 - 240 - 4000, 11009 - 240,      700, -775 - 240-3500);
	g_obj[2].setRotation(3.14159f, 0, 1, 0);
	g_obj[3].worldLocation(11009 - 240 - 1500, 1050, -775 - 240 - 3800);
	g_obj[3].scaleObject(1000.0f, 400, 700);
	g_obj[3].setRotation(-3.14159f/2, 1, 0, 0);
	g_obj[4].worldBoundingBox(8000-200, 0, -8255, 10000 - 200, 900, -7255);
	g_obj[4].setRotation(3.14159f, 0, 1, 0);
	g_obj[5].worldBoundingBox(8500 - 200, 0, -5700, 10500 - 200, 1800, -5200);
	g_obj[5].setRotation(3.14159f, 0, 1, 0);
	g_obj[6].worldBoundingBox(2800, 0.1f,   -8200, 4500, 800, -6500);
	g_obj[6].setRotation(3.14159f/2, 0, 1, 0);
	g_position = glm::vec3(12442 / 2.0f, 2500 / 2.0f, -8385 / 2.0f);
	g_obj[7].worldBoundingBox(1800, 0.1f, -8200, 2600, 600, -7400);
	g_obj[7].setRotation(3.14159f / 2, 0, 1, 0);
	g_obj[8].worldBoundingBox(7312-600, 0.1f, -7300, 7312, 800, -7300 +800);
	g_obj[8].setRotation(-3.14159f / 2, 0, 1, 0);
	g_obj[9].worldBoundingBox(7312 - 600, 0.1f, -8000, 7312, 800, -7400);
	g_obj[9].setRotation(-3.14159f / 2, 0, 1, 0);
	g_obj[10].worldBoundingBox(5400, 0.0f, -8255, 5500, 2000.0f, -6400);
	g_obj[10].setRotation(3.14159f, 0, 1, 0);
	g_obj[11].worldBoundingBox(1800, 0.0f, -3197, 1900, 2000.0f, -1940);
	g_obj[12].worldBoundingBox(2660 - 500, 0.1f, -3197, 2660, 800, -3197+500);
	g_obj[13].worldBoundingBox(2660 - 500, 0.1f, -1940-500, 2660, 800, -1940);
	g_obj[13].setRotation(-3.14159f, 0, 1, 0);
	g_obj[14].worldBoundingBox(4847-700, 0.0f, -130 - 700, 4847, 1000, -130);
	g_obj[14].setRotation(-3.14159f, 0, 1, 0);
	g_obj[15].worldBoundingBox(4047 - 1400, 0.0f, -130 - 600, 4047, 1000, -110);
	g_obj[15].setRotation(-3.14159f, 0, 1, 0);
	g_obj[16].worldBoundingBox(4047 - 1400 - 700, 0.0f, -130 - 700, 4047 - 1400, 1700, -130);
	g_obj[16].setRotation(-3.14159f, 0, 1, 0);
	g_obj[17].worldBoundingBox(6550-1000, 0.0f, -2800-700/2.0f,	6550 + 1000, 900, -2800 + 700 / 2.0f);
	g_obj[17].setRotation(-3.14159f/2, 0, 1, 0);
	g_obj[18].worldBoundingBox(4500 -750, 0.1f, -2800-700/2, 4500 +750, 400, -2800 + 700 / 2);
	g_obj[18].setRotation(3.14159f / 2, 0, 1, 0);
	g_obj[19].worldBoundingBox(4500 - 550, 400+300, -2800 - 700 / 2, 4500 + 550, 600+300, -2800 + 700 / 2);
	g_obj[19].setEuler(-3.14159f / 2, +3.14159f / 2, 0);
	g_obj[20].worldBoundingBox(1117, 400, -7555, 1247, 2500-400, -6000);
	g_obj[20].setEuler(0, 3.14159f, 0);
	g_obj[21].worldLocation( (5800+6800)/2.0f, (400+2100)/2.0f, (-8385 - 8255)/2.0f);
	g_obj[21].scaleObject(130, 2100-400, 1000);
	g_obj[21].setEuler(0, 3.14159f/2, 0);
	g_obj[22].worldLocation((1117 + 1247) / 2.0f, (400 + 2500) / 2.0f, (-5055 - 3327) / 2.0f);
	g_obj[22].scaleObject(-3327 - (-5055)+240, 2500-400, 1247 - 1117 );
	g_obj[22].setEuler(0, 3.14159f / 2, 0);

	// updating the collision map with the scene objects
	for (int i = 1; i < N_OBJECTS; i++)
	{
		SBox b;
		b.m_pMin.x = g_obj[i].m_position->x - g_obj[i].m_size->x / 2.0f;
		b.m_pMax.x = g_obj[i].m_position->x + g_obj[i].m_size->x / 2.0f;

		b.m_pMin.y = g_obj[i].m_position->y - g_obj[i].m_size->y / 2.0f;
		b.m_pMax.y = g_obj[i].m_position->y + g_obj[i].m_size->y / 2.0f;

		b.m_pMin.z = g_obj[i].m_position->z - g_obj[i].m_size->z / 2.0f;
		b.m_pMax.z = g_obj[i].m_position->z + g_obj[i].m_size->z / 2.0f;
		g_collMap.addBox(b);
	}

	// updating the colision map with the boxes of the house
	FILE *f = fopen("data.txt", "rt");
	if (f == NULL)
	{
		printf("Error loading data.txt\n");
		return 0;
	}
	char line[1024];
	while (fgets(line, 1024, f))
	{
		SBox b;
		if (fscanf(f, "%f %f %f %f %f %f", &b.m_pMin.x, &b.m_pMax.z, &b.m_pMin.y, &b.m_pMax.x, &b.m_pMin.z, &b.m_pMax.y) != 6)
			continue;
		b.m_pMax.z = -b.m_pMax.z;
		b.m_pMin.z = -b.m_pMin.z;
		g_collMap.addBox(b);
	}
	fclose(f);
	printf("collision map has been created\n");
	
	// glut callbacks!
	glutDisplayFunc(drawCallback);
	glutIdleFunc(drawCallback);
	glutReshapeFunc(reshapeCallback);
	glutKeyboardFunc(keyboardDown);
	glutSpecialFunc(specialKeyboardDown);
	glutSpecialUpFunc(specialKeyboardUp);
	glutMouseFunc(MouseLeftClick);
	glutMotionFunc(MouseMove);
	glutMainLoop();
	return 0;
}