#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#define GLM_FORCE_SWIZZLE
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "oglwindow.h"

//Initialize OpenGL window and run rendering loop
void OGLWindow::InitOGLWindow(std::unique_ptr<std::string> ufPath)
{
	OGLWindow::InitOGLWindowData();
    // Initialize GLFW library
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create OpenGL Rendering Context and make it current
	std::string strTitle = "View STL File: " + *ufPath;
	_window = glfwCreateWindow(_scrX, _scrY, strTitle.c_str(), NULL, NULL);
    if (_window == NULL)
    {
        std::cout << "Could not create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
	glfwSetWindowPos(_window, _scrLeft, _scrTop);
    glfwMakeContextCurrent(_window);

	// Setup User input
	OGLWindow::SetCallbackFunctions();
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Could not initialize GLAD" << std::endl;
        return;
    }

    glEnable(GL_DEPTH_TEST);

    OGLWindow::InitShaders();
	OGLWindow::InitFreeType();
	OGLWindow::SendVerticesToGPU(std::move(ufPath));

	// Main render loop
    while (!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();		//was last in this loop
        OGLWindow::RenderScene();
    	glfwSwapBuffers(_window);
    }

    glfwTerminate();
    return;
}

void OGLWindow::InitOGLWindowData()
{
	int i;
	_cam.SetDistance(10.0f);
	_cam.UpdateCameraVectors(_viewX, _viewY, _pan.x, _pan.y);
	_ob.emplace_back();		//_ob[0] - single arrow for triad in bottom left corner
	_ob[0].scale = glm::vec3(0.2f, 0.2f, 0.2f);
	_ob.emplace_back();		//_ob[1] - object from vertices in stl file

	// Initialize 2D vector<float> _msMult which is all 0.0f, except the main diagonal
	//   which is a factor to multiply mouse movements (in pixels) by to yield changes
	//   to view control variables like angles and displacements. For example:
	//     _msMult[1][1] = 0.1f, so 10 pixels of mouse movement changes _viewY by 1.0 degree
	//   see OGLWindow::MousePositionCallback for calculation
	//   This scenario avoids using many if statements in that calculation
	vector<float> vTemp;
    for (i = 0; i < 16; i++)
    {
		vTemp.emplace_back(0.0f);
	}
    for (i = 0; i < 16; i++)
    {
		_msMult.emplace_back(vTemp);
	}
    for (i = 0; i < 16; i++)
    {
		_msMult[i][i] = 0.1f;
	}
    _msMult[0][0] = 0.0f;
    _msMult[3][3] = 0.025f;
    _msMult[4][4] = 0.025f;
    _msMult[5][5] = 0.025f;
    _msMult[6][6] = 0.01f;

	Material matTemp;
	matTemp.name = "Aluminum";
	matTemp.diffuse = glm::vec3(0.55f, 0.5f, 0.5f);
	matTemp.specular = glm::vec3(0.8f, 0.7f, 0.7f);
	matTemp.alpha = 1.0f;
	matTemp.shininess = 51.2f;
    _matl.emplace_back(matTemp);

	matTemp.name = "Red Plastic";
	matTemp.diffuse = glm::vec3(0.7f, 0.0f, 0.0f);
	matTemp.specular = glm::vec3(0.7f, 0.6f, 0.6f);
	matTemp.alpha = 1.0f;
	matTemp.shininess = 32.0f;
    _matl.emplace_back(matTemp);

	matTemp.name = "Green Plastic";
	matTemp.diffuse = glm::vec3(0.0f, 0.7f, 0.0f);
	matTemp.specular = glm::vec3(0.6f, 0.6f, 0.7f);
	matTemp.alpha = 1.0f;
	matTemp.shininess = 32.0f;
    _matl.emplace_back(matTemp);

	matTemp.name = "Blue Plastic";
	matTemp.diffuse = glm::vec3(0.0f, 0.0f, 0.7f);
	matTemp.specular = glm::vec3(0.6f, 0.7f, 0.6f);
	matTemp.alpha = 1.0f;
	matTemp.shininess = 32.0f;
    _matl.emplace_back(matTemp);

	matTemp.name = "Black Plastic";
	matTemp.diffuse = glm::vec3(0.1f, 0.1f, 0.1f);
	matTemp.specular = glm::vec3(0.2f, 0.2f, 0.2f);
	matTemp.alpha = 1.0f;
	matTemp.shininess = 32.0f;
    _matl.emplace_back(matTemp);
}

void OGLWindow::InitShaders()
{
	// Initialize vector of Shader objects
	_shd.emplace_back();
    _shd[0].RetrieveAndCompile("../res/shaders/clrmatl.vert", "../res/shaders/clrmatl.frag");  //shd_ClrMatl
	_shd.emplace_back();
    _shd[1].RetrieveAndCompile("../res/shaders/text.vert", "../res/shaders/text.frag");        //shd_Text
}

void OGLWindow::SendVerticesToGPU(std::unique_ptr<std::string> ufPath)
{	
	// Use move semantics to send .stl filename to Shape object
	//   This stl file is a single arrow to be repeated three times in ccordinate triad
	//   at bottom left of OpenGL window
	std::unique_ptr arrowfPath = std::make_unique<std::string>("../res/stl/TriadArrow-01.stl");
	Shape triadArrow;
	triadArrow.OpenSTL(std::move(arrowfPath));

	// Use move semantics to send user selected .stl filename to Shape object
	Shape surfaceMesh;
	surfaceMesh.OpenSTL(std::move(ufPath));

	// Get geometric extents of obect to set object offset and scale
	glm::vec3 stlMin = surfaceMesh.minxyz;
	glm::vec3 stlMax = surfaceMesh.maxxyz;
	glm::vec3 center = glm::vec3((stlMin + stlMax) * 0.5f);
	float rangeX = stlMax.x - stlMin.x;
	float rangeY = stlMax.y - stlMin.y;
	float rangeZ = stlMax.z - stlMin.z;
	float rangeMax = std::max(std::max(rangeX, rangeY), rangeZ);
	float scale = 25.0f / rangeMax;
	_ob[1].scale = glm::vec3(scale, scale, scale);
	_ob[1].pos = center * (-1.0f);

	// Send all vertices to GPU
    OGLWindow::InitVertices(_ob[0], triadArrow);			//for coordinate triad
    OGLWindow::InitVertices(_ob[1], surfaceMesh);	//for view with lighting
}

void OGLWindow::InitVertices(OGLObject &obj, Shape &shp)
{
    float *vertices;
    long vc;    //vertices counter

    obj.indexCount = 3 * shp.facets.size();	//Number of points - 3 per facet
    obj.vertexCount = 6 * obj.indexCount;	//Number of values - 6 per point
    vertices = new float[obj.vertexCount];
	vc = 0;
	for (auto f : shp.facets)
	{
        for (int p = 0; p < 3; p++)	//count points of vertex
        {
            for (int i = 0; i < 3; i++)	//count x, y, z element
            {
	            vertices[vc] = f.vtx[p][i]; 
				vc++;
			}
            for (int j = 0; j < 3; j++)	//count x, y, z element
            {
	            vertices[vc] = f.ijk[j]; 
				vc++;
			}
        }
	}

    glGenVertexArrays(1, &obj.vaoIdx);
    glGenBuffers(1, &obj.vboIdx);

    glBindBuffer(GL_ARRAY_BUFFER, obj.vboIdx);
    glBufferData(GL_ARRAY_BUFFER, obj.vertexCount * sizeof(float), vertices, GL_DYNAMIC_DRAW);

    glBindVertexArray(obj.vaoIdx);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

    delete[] vertices;
}

void OGLWindow::InitFreeType()
{
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
	}

	std::string font_name = "../res/fonts/DejaVuSans.ttf";	//"../res/fonts/NotoMono-Regular.ttf";
	if (font_name.empty())
	{
		std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
	}

	FT_Face face;
	if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
	}
	else {
		FT_Set_Pixel_Sizes(face, 0, 48);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		for (unsigned char c = 0; c < 128; c++)
		{
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
				continue;
			}
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				static_cast<unsigned int>(face->glyph->advance.x)
			};
			_Characters.insert(std::pair<char, Character>(c, character));
		}
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	glGenVertexArrays(1, &_txtVAO);
	glGenBuffers(1, &_txtVBO);
	glBindVertexArray(_txtVAO);
	glBindBuffer(GL_ARRAY_BUFFER, _txtVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void OGLWindow::RenderText(std::string txt, float x, float y, float scale, glm::vec3 color)
{
	_shd[1].use();
	_shd[1].setVec3("textColor", color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(_txtVAO);

	std::string::const_iterator c;
	for (c = txt.begin(); c != txt.end(); c++)
	{
		Character ch = _Characters[*c];

		float xpos = x + ch.Bearing.x * scale;
		float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		glBindBuffer(GL_ARRAY_BUFFER, _txtVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		x += (ch.Advance >> 6) * scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void OGLWindow::ApplyMatlToShader(unsigned int matID, Shader &shd)
{
	// Used in RenderScene() to send material properties to a "Uniforms" in Shader
	shd.setVec3("material.diffuse", _matl[matID].diffuse);
	shd.setVec3("material.specular", _matl[matID].specular);
	shd.setFloat("material.alpha", _matl[matID].alpha);
	shd.setFloat("material.shininess", _matl[matID].shininess);
}

void OGLWindow::SetProjectionAndView(Shader &shd, bool &withpan, bool &withZoom, glm::vec3 &projTran, glm::vec3 &viewTran)
{
	float orthoX = (10.0f * _scrX / _scrY);
	float orthoY = (10.0f);
	float orthoZ = 100.0f;
	glm::vec3 oneVec3 = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::mat4 projection = glm::ortho(-orthoX, orthoX, -orthoY, orthoY, -orthoZ, orthoZ);
	projection = glm::translate(projection, projTran);
	shd.setMat4("projection", projection);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 camview = glm::mat4(1.0f);
	glm::mat4 zoomview = glm::mat4(1.0f);
	if (withpan)
	{
		camview = glm::lookAt(_cam.GetPosition(), _cam.GetTarget(), _cam.GetUp());
	}
	else
	{
		camview = glm::lookAt(_cam.GetPositionNoPan(), _cam.GetTargetNoPan(), _cam.GetUp());
	}
	if (withZoom)
	{
		glm::vec3 v3Temp = glm::vec3(oneVec3 * _zoom);
		zoomview = glm::scale(zoomview, v3Temp);
	}
	view = zoomview * camview;
	view = glm::translate(view, viewTran);
	shd.setMat4("view", view);
}

void OGLWindow::RenderScene()
{
	bool bTrue = true;
	bool bFalse = false;

	// time between frames
	float currentFrame = glfwGetTime();
	_deltaTime = currentFrame - _lastFrame;
	_lastFrame = currentFrame;

	//Render
	glClearColor(_GLBackground[0], _GLBackground[1], _GLBackground[2], _GLBackground[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_FRONT, GL_FILL);

	float orthoX = 10.0f * _scrX / _scrY;
	float orthoY = 10.0f;
	float orthoZ = 100.0f;
	glm::vec3 zeroVec3 = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 oneVec3 = glm::vec3(1.0f, 1.0f, 1.0f);

	_shd[0].use();
	_shd[0].setVec3("viewPos", _cam.GetPosition());
	_shd[0].setVec3("viewTarg", _cam.GetTarget());

	_shd[0].setVec3("dirLight.ambient", _LightAmbient[0], _LightAmbient[1], _LightAmbient[2]);
	_shd[0].setVec3("dirLight.diffuse", _LightDiffuse[0], _LightDiffuse[1], _LightDiffuse[2]);
	_shd[0].setVec3("dirLight.specular", _LightSpecular[0], _LightSpecular[1], _LightSpecular[2]);
	//Check that at least one of option bools (0 or 1) is set
	//	opt 0 - Draw Object
	//	opt 1 - Draw Lines around Triangles
	glm::mat4 model = glm::mat4(1.0);
	if (_ob[1].opt[0])
	{
		_shd[0].use();
		//Direction of columated lighting
		_shd[0].setVec3("dirLight.direction", _LightDirection[0], _LightDirection[1], _LightDirection[2]);

		//Set Projection and View Matrices (except for Arrows)
		OGLWindow::SetProjectionAndView(_shd[0], bTrue, bTrue, zeroVec3, zeroVec3);
		glBindVertexArray(_ob[1].vaoIdx);
		//Set Model Matrix
		model = glm::translate(model, _ob[1].pos);
		model = glm::rotate(model, glm::radians(_ob[1].rotang.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::rotate(model, glm::radians(_ob[1].rotang.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(_ob[1].rotang.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, _ob[1].scale);
		_shd[0].setMat4("model", model);

		//Apply material settings (OpenGL view materials)
		OGLWindow::ApplyMatlToShader(_ob[1].matlIdx, _shd[0]);

		//Render vertices i.a.w. StatusBit
		//	Bit 0 - Draw filled Triangles
		if (_ob[1].opt[0])
		{
			glDrawArrays(GL_TRIANGLES, 0, _ob[1].indexCount);
		}
		//	Bit 1 - Draw Lines around Triangles
		if (_ob[1].opt[1])
		{
			// Implement polygon offsets for mesh lines
			//   Use polygon offset so mesh lines do not disappear into surface
			glm::vec3 polyoff = 0.001f * (_cam.GetPosition() - _cam.GetTarget());
			OGLWindow::SetProjectionAndView(_shd[0], bTrue, bTrue, zeroVec3, polyoff);
			OGLWindow::ApplyMatlToShader(4, _shd[0]);	//Change to 4 for black
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDrawArrays(GL_TRIANGLES, 0, _ob[1].indexCount);
			// Return to polygon fill mode
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	// Draw coordinate triad
	if (_ob[0].opt[0])
	{	
		_shd[0].use();
		glm::vec3 projTranslate = glm::vec3(-orthoX + 2.0f, -orthoY + 1.0f, 0.0f);
		// Disable zoom and pan for triad
		OGLWindow::SetProjectionAndView(_shd[0], bFalse, bFalse, projTranslate, zeroVec3);

		//Specific Light Direction for triad
		_shd[0].setVec3("dirLight.direction", -0.25f, 0.75f, 0.26f);
		glBindVertexArray(_ob[0].vaoIdx);

		// X-Axis (red)
		model = glm::mat4(1.0);
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, _ob[0].scale);
		_shd[0].setMat4("model", model);
		OGLWindow::ApplyMatlToShader(1, _shd[0]);
		glDrawArrays(GL_TRIANGLES, 0, _ob[0].indexCount);
		
		// Y-Axis (green)
		model = glm::mat4(1.0);
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, _ob[0].scale);
		_shd[0].setMat4("model", model);
		OGLWindow::ApplyMatlToShader(2, _shd[0]);
		glDrawArrays(GL_TRIANGLES, 0, _ob[0].indexCount);
		
		// Z-Axis (blue)
		model = glm::mat4(1.0);
		model = glm::scale(model, _ob[0].scale);
		_shd[0].setMat4("model", model);
		OGLWindow::ApplyMatlToShader(3, _shd[0]);
		glDrawArrays(GL_TRIANGLES, 0, _ob[0].indexCount);
	}
    
	// Enable Blending for rendering text
	//   Blending allows transparency.  Each character is a transparent rectangle
	//   with an opaque glyph inside.  If we did not enable blending then the
	//   rectangle surrounding each character would be visible
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Draw Text
	_shd[1].use();
	glm::mat4 projection = glm::ortho(-orthoX, orthoX, -orthoY, orthoY, -orthoZ, orthoZ);
	_shd[1].setMat4("projection", projection);

	OGLWindow::RT_TextAndFloat("View Angle X: ", _viewX, 1, 1);
	OGLWindow::RT_TextAndFloat("View Angle Y: ", _viewY, 1, 2);
	OGLWindow::RT_TextAndFloat("Zoom: ", _zoom, 1, 3);
	OGLWindow::RT_TextAndFloat("Rot. Angle X: ", _ob[1].rotang.x, 1, 4);
	OGLWindow::RT_TextAndFloat("Rot. Angle Y: ", _ob[1].rotang.y, 1, 5);
	OGLWindow::RT_TextAndFloat("Rot. Angle Z: ", _ob[1].rotang.z, 1, 6);
	OGLWindow::RT_TextAndFloat("Pos. X (i): ", _ob[1].pos.x, 1, 7);
	OGLWindow::RT_TextAndFloat("Pos. Y (j): ", _ob[1].pos.y, 1, 8);
	OGLWindow::RT_TextAndFloat("Pos. Z (k): ", _ob[1].pos.z, 1, 9);
	OGLWindow::RT_TextAndFloat("Frame time: ", _deltaTime, 3, 10);
	OGLWindow::RT_TextAndFloat("Mouse X: ", (float)_msx, 0, 11);
	OGLWindow::RT_TextAndFloat("Mouse Y: ", (float)_msy, 0, 12);
	
	//Disable Blending for next render cycle
	glDisable(GL_BLEND);
}

void OGLWindow::RT_TextAndFloat(std::string txt, float val, int prec, int row)
{
	float orthoX = 10.0f * _scrX / _scrY;
	float orthoY = 10.0f;
	float offright = 3.0f;
	float offdown = 0.7f;
	float txtscale = 0.008;
	char buff[16];
	std::string strTemp = "%3." + std::to_string(prec) + "f";
	const char *fprec = strTemp.c_str();
	memset(buff, 0, sizeof(buff));
	OGLWindow::RenderText(txt, (-orthoX + 0.25f), (orthoY - (row * offdown)), txtscale, glm::vec3(1.0f, 1.0f, 0.1f));
	sprintf(buff, fprec, val);
	std::string sbuff = buff;
	OGLWindow::RenderText(sbuff, (-orthoX + 0.25f + offright), (orthoY - (row * offdown)), txtscale, glm::vec3(1.0f, 1.0f, 0.1f));
}

//==========User Input Callbacks=========================================

void OGLWindow::SetCallbackFunctions()
{
    GLFWCallbackWrapper::SetApplication(this);
	glfwSetFramebufferSizeCallback(_window, GLFWCallbackWrapper::ScreenSizeCallback);
    glfwSetCursorPosCallback(_window, GLFWCallbackWrapper::MousePositionCallback);
	glfwSetMouseButtonCallback(_window, GLFWCallbackWrapper::MouseButtonCallback);
    glfwSetKeyCallback(_window, GLFWCallbackWrapper::KeyboardCallback);
}

void OGLWindow::GLFWCallbackWrapper::ScreenSizeCallback(GLFWwindow* window, int width, int height)
{
    s_oglwindow->ScreenSizeCallback(window, width, height);
}

void OGLWindow::GLFWCallbackWrapper::MousePositionCallback(GLFWwindow* window, double positionX, double positionY)
{
	int posX = (int)round(positionX);
	int posY = (int)round(positionY);
    s_oglwindow->MousePositionCallback(window, posX, posY);
}

void OGLWindow::GLFWCallbackWrapper::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    s_oglwindow->MouseButtonCallback(window, button, action, mods);
}

void OGLWindow::GLFWCallbackWrapper::KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    s_oglwindow->KeyboardCallback(window, key, scancode, action, mods);
}

void OGLWindow::GLFWCallbackWrapper::SetApplication(OGLWindow* application)
{
    GLFWCallbackWrapper::s_oglwindow = application;
}

OGLWindow* OGLWindow::GLFWCallbackWrapper::s_oglwindow = nullptr;

// Adjust variables when user manipulates screen size
void OGLWindow::ScreenSizeCallback(GLFWwindow* window, int width, int height)
{
	_scrX = width;
	_scrY = height;	
    glViewport(0, 0, width, height);
}

// Respond to user mouse movements
void OGLWindow::MousePositionCallback(GLFWwindow* window, int xpos, int ypos)
{
    int movex;
    int movey;
    _msx = xpos;
    _msy = ypos;
    if (_leftButtonDown)
    {
        movex = _leftDownPosX - xpos;
        movey = _leftDownPosY - ypos;
        _leftDownPosX = xpos;
        _leftDownPosY = ypos;

        _viewY += movex * _msMult[_msIdX][1] + movey * _msMult[_msIdY][1];
        _viewX += movex * _msMult[_msIdX][2] + movey * _msMult[_msIdY][2];
        _zoom *= 1 - (movex * _msMult[_msIdX][6] + movey * _msMult[_msIdY][6]);
        _pan.x += (movex * _msMult[_msIdX][3] + movey * _msMult[_msIdY][3]) / _zoom;
        _pan.y -= (movex * _msMult[_msIdX][4] + movey * _msMult[_msIdY][4]) / _zoom;
        _pan.z += (movex * _msMult[_msIdX][5] + movey * _msMult[_msIdY][5]) / _zoom;
    	_ob[1].rotang.x -= movex * _msMult[_msIdX][7] + movey * _msMult[_msIdY][7];
        _ob[1].rotang.y -= movex * _msMult[_msIdX][8] + movey * _msMult[_msIdY][8];
        _ob[1].rotang.z -= movex * _msMult[_msIdX][9] + movey * _msMult[_msIdY][9];
        _ob[1].pos.x -= (movex * _msMult[_msIdX][10] + movey * _msMult[_msIdY][10]) / _zoom;
        _ob[1].pos.y -= (movex * _msMult[_msIdX][11] + movey * _msMult[_msIdY][11]) / _zoom;
        _ob[1].pos.z -= (movex * _msMult[_msIdX][12] + movey * _msMult[_msIdY][12]) / _zoom;
        _ob[1].scale.x += (movex * _msMult[_msIdX][13] + movey * _msMult[_msIdY][13]) / _zoom;
        _ob[1].scale.y += (movex * _msMult[_msIdX][14] + movey * _msMult[_msIdY][14]) / _zoom;
        _ob[1].scale.z += (movex * _msMult[_msIdX][15] + movey * _msMult[_msIdY][15]) / _zoom;

		_cam.UpdateCameraVectors(_viewX, _viewY, _pan.x, _pan.y);
    }
}

// Respond to user mouse button press
void OGLWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        _leftButtonDown = true;
        _leftDownPosX = _msx;
        _leftDownPosY = _msy;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        _leftButtonDown = false;
    }
}

//Respond to user key strokes
void OGLWindow::KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (_keyDown > 0)
	{
		switch (_keyDown)
		{
			case GLFW_KEY_LEFT_SHIFT:
				if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) OGLWindow::KeysUp();
				break;
			case GLFW_KEY_LEFT_CONTROL:
				if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE) OGLWindow::KeysUp();
				break;
			case GLFW_KEY_I:
				if (glfwGetKey(window, GLFW_KEY_I) == GLFW_RELEASE) OGLWindow::KeysUp();
				break;
			case GLFW_KEY_J:
				if (glfwGetKey(window, GLFW_KEY_J) == GLFW_RELEASE) OGLWindow::KeysUp();
				break;
			case GLFW_KEY_K:
				if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE) OGLWindow::KeysUp();
				break;
			case GLFW_KEY_M:
				if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE)
				{
					OGLWindow::ToggleMesh();
					OGLWindow::KeysUp();
				}
				break;
			case GLFW_KEY_O:
				if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE)
				{
					OGLWindow::ReturnToOrigin();
					OGLWindow::KeysUp();
				}
				break;
			case GLFW_KEY_X:
				if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE) OGLWindow::KeysUp();
				break;
			case GLFW_KEY_Y:
				if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_RELEASE) OGLWindow::KeysUp();
				break;
			case GLFW_KEY_Z:
				if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE) OGLWindow::KeysUp();
				break;
		}
	}
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
        glfwSetWindowShouldClose(window, true);
	}

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		_msIdX = 6;
		_msIdY = 0;
		_keyDown = GLFW_KEY_LEFT_SHIFT;
	}

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		_msIdX = 3;
		_msIdY = 4;
		_keyDown = GLFW_KEY_LEFT_CONTROL;
	}   

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
	{
		_keyDown = GLFW_KEY_B;
	}   

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
	{
		_keyDown = GLFW_KEY_C;
	}  

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
	{
		_msIdX = 10;
		_msIdY = 0;
		_keyDown = GLFW_KEY_I;
	}  

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
	{
		_msIdX = 11;
		_msIdY = 0;
		_keyDown = GLFW_KEY_J;
	}  

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
	{
		_msIdX = 12;
		_msIdY = 0;
		_keyDown = GLFW_KEY_K;
	}  

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
		_keyDown = GLFW_KEY_M;
    }

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
    {
		_keyDown = GLFW_KEY_O;
    }

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
    {
        std::cout << _msx << ", " << _msy << "\n";
    }

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
	{
		_msIdX = 7;
		_msIdY = 0;
		_keyDown = GLFW_KEY_X;
	}  

    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
	{
		_msIdX = 8;
		_msIdY = 0;
		_keyDown = GLFW_KEY_Y;
	}  

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
	{
		_msIdX = 9;
		_msIdY = 0;
		_keyDown = GLFW_KEY_Z;
	} 
}

void OGLWindow::KeysUp()
{
	_msIdX = 2;
	_msIdY = 1;
	_keyDown = 0;
}

// Make solid mesh visible or hidden
void OGLWindow::ToggleMesh()
{
	bool bTemp = _ob[1].opt[1];
	if (bTemp)
	{
        _ob[1].opt[1] = false;
	}
	else
	{
        _ob[1].opt[1] = true;
	}
}

// Move main shape to default view angles and zero displacement and rotation
void OGLWindow::ReturnToOrigin()
{
	_pan.x = 0.0f;
	_pan.y = 0.0f;
	_pan.z = 0.0f;
	_zoom = 1.0f;
	_ob[1].rotang.x = 0.0f;
	_ob[1].rotang.y = 0.0f;
	_ob[1].rotang.z = 0.0f;
	_ob[1].pos.x = 0.0f;
	_ob[1].pos.y = 0.0f;
	_ob[1].pos.z = 0.0f;
}

/*
#include <algorithm>
#include <thread>

void OGLWindow::InitOGLWindow(int thcount, int thmax, std::unique_ptr<std::string> ufPath)
{
	OGLWindow::InitOGLWindowData();
    // Initialize GLFW library
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create OpenGL Rendering Context and make it current
	OGLWindow::WinSizeAndPos(thcount, thmax);
	std::string strTitle = "View STL File" + std::to_string(thcount);
	_window = glfwCreateWindow(_scrX, _scrY, strTitle.c_str(), NULL, NULL);
    if (_window == NULL)
    {
        std::cout << "Could not create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
	glfwSetWindowPos(_window, _scrLeft, _scrTop);
    glfwMakeContextCurrent(_window);

	// Setup User input
	if (thcount == 0)
	{
		OGLWindow::SetCallbackFunctions();
	}
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// Load GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Could not initialize GLAD" << std::endl;
        return;
    }

    glEnable(GL_DEPTH_TEST);

    OGLWindow::InitShaders();
	OGLWindow::InitFreeType();
	OGLWindow::SendVerticesToGPU(std::move(ufPath));

    while (!glfwWindowShouldClose(_window))
    {
        if (thcount == 0) glfwPollEvents();		//was last in this loop
		OGLWindow::ProcessView(thcount);
        OGLWindow::RenderScene();
    	glfwSwapBuffers(_window);
    }

    glfwTerminate();
    return;
}

void OGLWindow::WinSizeAndPos(int thcount, int thmax)
{
	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	int dimx = mode->width * 0.8 / thmax;
	int dimy = mode->height * 0.8;
	int dimMin = dimx;
	if (dimy < dimx) dimMin = dimy;
	if (dimMin > 480) dimMin = 480;
	_scrX = dimMin;
	_scrY = dimMin;
	_scrTop = dimMin * 0.2;
	_scrLeft = dimMin * 0.2 + thcount * (_scrX + 5);
}

void OGLWindow::ProcessView(int thcount)
{
	if (thcount == 0)
	{
		_viewX = _viewX + 0.01;	
	}
	
	//_vc = _message.receive();
	//_viewX = _vc.GetViewX();
	//_viewY = _vc.GetViewY();
	//_zoom = _vc.GetZoom();
	//_pan = _vc.GetPan();
	//_message.send(std::move(_vc));
	
}

// Implementation of class "MessageQueue" from Traffic Simulator

template <typename T>
T MessageQueue<T>::receive()
{
    //Lock prior to accessing
    std::unique_lock<std::mutex> uLock(_mutex);
    //Wait for new message
    _cond.wait(uLock, [this] { return !_queue.empty(); });
    //Use move semantics to remove message from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    //Lock prior to accessing
    std::lock_guard<std::mutex> uLock(_mutex);
    //Remove old messages
    _queue.clear();
    //Add new message to queue
    _queue.emplace_back(std::move(msg));
    _cond.notify_one();
    //std::cout << " Message " << msg << " has been added to the queue" << std::endl;
}

From MousePositionCallback()
		std::unique_lock<std::mutex> vwLock(_mutex);

		_vc.SetViewX(_viewX);
		_vc.SetViewY(_viewY);
		_vc.SetZoom(_zoom);
		_vc.SetPan(_pan);
		_message.send(std::move(_vc));
*/