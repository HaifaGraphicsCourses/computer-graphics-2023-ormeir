#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <iostream>

#include "Renderer.h"
#include "InitShader.h"
#include <glm/gtx/transform.hpp>

#define INDEX(width,x,y,c) ((x)+(y)*(width))*3+(c)
#define Z_INDEX(width,x,y) ((x)+(y)*(width))

Renderer::Renderer(int viewport_width, int viewport_height) :
	viewport_width(viewport_width),
	viewport_height(viewport_height)
{
	InitOpenglRendering();
	CreateBuffers(viewport_width, viewport_height);
}

Renderer::~Renderer()
{
	delete[] color_buffer;
}

void Renderer::PutPixel(int i, int j, const glm::vec3& color)
{
	if (i < 0) return; if (i >= viewport_width) return;
	if (j < 0) return; if (j >= viewport_height) return;
	
	color_buffer[INDEX(viewport_width, i, j, 0)] = color.x;
	color_buffer[INDEX(viewport_width, i, j, 1)] = color.y;
	color_buffer[INDEX(viewport_width, i, j, 2)] = color.z;
}

void Renderer::DrawLine(const glm::ivec2& p1, const glm::ivec2& p2, const glm::vec3& color)
{
	
	glm::ivec2 start = glm::ivec2(p1);
	glm::ivec2 end = glm::ivec2(p2);
	if (p1.x > p2.x)
	{
		start = glm::ivec2(p2);
		end = glm::ivec2(p1);
	}
	int transform1 = 1, transform2 = 1;
	int* coordinate1 = &start.y;
	int* coordinate2 = &start.x;
	int firstDelta = (end.x - start.x);
	int secondDelta = (end.y - start.y);

	double m = ((double)secondDelta / (double)firstDelta);

	int theEnd = end.x;
	int* theStart = &start.x;
	int firstDeltaChange = 1;
	int secondDeltaChange = 1;
	int e = 0;

	if(m < -1 || m > 1)
	{
		coordinate1 = &start.x;
		coordinate2 = &start.y;
		theEnd = end.y;
		theStart = &start.y;
		swap(firstDelta, secondDelta);

		if (m < -1)
		{
			transform2 = -1;
			firstDeltaChange = -1;
		}
	}
	if (m<=0 && m>=-1)
	{
		transform1 = -1;
		secondDeltaChange = -1;
	}

	while (*theStart != theEnd)
	{
		if (e > 0)
		{
			*coordinate1 += transform1;
			e -= 2 * (firstDelta)*firstDeltaChange;
		}
		PutPixel(start.x, start.y, color);
		*coordinate2 += transform2;	
		e += 2*(secondDelta)*secondDeltaChange;	
	}

}

void Renderer::CreateBuffers(int w, int h)
{
	CreateOpenglBuffer(); //Do not remove this line.
	color_buffer = new float[3 * w * h];
	ClearColorBuffer(glm::vec3(0.0f, 0.0f, 0.0f));
}

//##############################
//##OpenGL stuff. Don't touch.##
//##############################

// Basic tutorial on how opengl works:
// http://www.opengl-tutorial.org/beginners-tutorials/tutorial-2-the-first-triangle/
// don't linger here for now, we will have a few tutorials about opengl later.
void Renderer::InitOpenglRendering()
{
	// Creates a unique identifier for an opengl texture.
	glGenTextures(1, &gl_screen_tex);

	// Same for vertex array object (VAO). VAO is a set of buffers that describe a renderable object.
	glGenVertexArrays(1, &gl_screen_vtc);

	GLuint buffer;

	// Makes this VAO the current one.
	glBindVertexArray(gl_screen_vtc);

	// Creates a unique identifier for a buffer.
	glGenBuffers(1, &buffer);

	// (-1, 1)____(1, 1)
	//	     |\  |
	//	     | \ | <--- The exture is drawn over two triangles that stretch over the screen.
	//	     |__\|
	// (-1,-1)    (1,-1)
	const GLfloat vtc[]={
		-1, -1,
		 1, -1,
		-1,  1,
		-1,  1,
		 1, -1,
		 1,  1
	};

	const GLfloat tex[]={
		0,0,
		1,0,
		0,1,
		0,1,
		1,0,
		1,1};

	// Makes this buffer the current one.
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	// This is the opengl way for doing malloc on the gpu. 
	glBufferData(GL_ARRAY_BUFFER, sizeof(vtc)+sizeof(tex), NULL, GL_STATIC_DRAW);

	// memcopy vtc to buffer[0,sizeof(vtc)-1]
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vtc), vtc);

	// memcopy tex to buffer[sizeof(vtc),sizeof(vtc)+sizeof(tex)]
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vtc), sizeof(tex), tex);

	// Loads and compiles a sheder.
	GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );

	// Make this program the current one.
	glUseProgram(program);

	// Tells the shader where to look for the vertex position data, and the data dimensions.
	GLint  vPosition = glGetAttribLocation( program, "vPosition" );
	glEnableVertexAttribArray( vPosition );
	glVertexAttribPointer( vPosition,2,GL_FLOAT,GL_FALSE,0,0 );

	// Same for texture coordinates data.
	GLint  vTexCoord = glGetAttribLocation( program, "vTexCoord" );
	glEnableVertexAttribArray( vTexCoord );
	glVertexAttribPointer( vTexCoord,2,GL_FLOAT,GL_FALSE,0,(GLvoid *)sizeof(vtc) );

	//glProgramUniform1i( program, glGetUniformLocation(program, "texture"), 0 );

	// Tells the shader to use GL_TEXTURE0 as the texture id.
	glUniform1i(glGetUniformLocation(program, "texture"),0);
}

void Renderer::CreateOpenglBuffer()
{
	// Makes GL_TEXTURE0 the current active texture unit
	glActiveTexture(GL_TEXTURE0);

	// Makes glScreenTex (which was allocated earlier) the current texture.
	glBindTexture(GL_TEXTURE_2D, gl_screen_tex);

	// malloc for a texture on the gpu.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, viewport_width, viewport_height, 0, GL_RGB, GL_FLOAT, NULL);
	glViewport(0, 0, viewport_width, viewport_height);
}

void Renderer::SwapBuffers()
{
	// Makes GL_TEXTURE0 the current active texture unit
	glActiveTexture(GL_TEXTURE0);

	// Makes glScreenTex (which was allocated earlier) the current texture.
	glBindTexture(GL_TEXTURE_2D, gl_screen_tex);

	// memcopy's colorBuffer into the gpu.
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, viewport_width, viewport_height, GL_RGB, GL_FLOAT, color_buffer);

	// Tells opengl to use mipmapping
	glGenerateMipmap(GL_TEXTURE_2D);

	// Make glScreenVtc current VAO
	glBindVertexArray(gl_screen_vtc);

	// Finally renders the data.
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::ClearColorBuffer(const glm::vec3& color)
{
	for (int i = 0; i < viewport_width; i++)
	{
		for (int j = 0; j < viewport_height; j++)
		{
			PutPixel(i, j, color);
		}
	}
}


void Renderer::drawAxis(const Scene& scene)
{
	MeshModel model = scene.GetModel(0);

	glm::mat4x4 view = scene.getActiveCamera().getViewTransformation();
	glm::mat4 viewport = scene.getActiveCamera().view_port;
	glm::mat4 proj = scene.getActiveCamera().projection_transformation;

	//World Axis
	for (int i = 0; i < 3; i++)
	{
		glm::ivec2 start = glm::ivec2(viewport_width / 2+i, (viewport_height / 2)+i);
		glm::ivec2 y = glm::ivec2((viewport_width / 2)+i, (viewport_height / 2 )+ 400+i);
		glm::ivec2 x = glm::ivec2((viewport_width / 2) + 400+i, (viewport_height / 2)+i);
		DrawLine(start, y, glm::ivec3(1, 0, 0));
		DrawLine(start, x, glm::ivec3(1, 0, 0));
	}
	

	glm::ivec4 startM = glm::ivec4(0 , 0 , 0, 1);
	glm::ivec4 yM = glm::ivec4(0 , 1, 0, 1);
	glm::ivec4 xM = glm::ivec4(1, 0 , 0, 1);
	glm::ivec4 zM = glm::ivec4(0, 0, 1, 1);

	startM = viewport * proj * view * model.getWorldTransform() * startM;
	yM = viewport * proj * view * model.getWorldTransform() * yM;
	xM = viewport * proj * view * model.getWorldTransform() * xM;
	zM = viewport * proj * view * model.getWorldTransform() * zM;

	startM /= startM.w;
	yM /= yM.w;
	xM /= xM.w;
	zM /= zM.w;

	DrawLine(startM, yM, glm::ivec3(0, 0, 1));
	DrawLine(startM, xM, glm::ivec3(0, 0, 1));
	DrawLine(startM, zM, glm::ivec3(0, 0, 1));
}


void Renderer::drawBoundingBox(const Scene& scene)
{
	MeshModel model = scene.GetActiveModel();
	glm::mat4x4 view = scene.getActiveCamera().getViewTransformation();
	glm::mat4 viewport = scene.getActiveCamera().view_port;
	glm::mat4 proj = scene.getActiveCamera().projection_transformation;

	glm::vec4 rdf = glm::vec4(model.maxX, model.minY, model.maxZ, 1);
	glm::vec4 ruf = glm::vec4(model.maxX, model.maxY, model.maxZ, 1);
	glm::vec4 ldf = glm::vec4(model.minX, model.minY, model.maxZ, 1);
	glm::vec4 luf = glm::vec4(model.minX, model.maxY, model.maxZ, 1);
	glm::vec4 rdb = glm::vec4(model.maxX, model.minY, model.minZ, 1);
	glm::vec4 rub = glm::vec4(model.maxX, model.maxY, model.minZ, 1);
	glm::vec4 ldb = glm::vec4(model.minX, model.minY, model.minZ, 1);
	glm::vec4 lub = glm::vec4(model.minX, model.maxY, model.minZ, 1);

	rdf = viewport * proj * view * model.transform(rdf); rdf /= rdf.w;
	ruf = viewport * proj * view * model.transform(ruf); ruf /= ruf.w;
	ldf = viewport * proj * view * model.transform(ldf); ldf /= ldf.w;
	luf = viewport * proj * view * model.transform(luf); luf /= luf.w;
	rdb = viewport * proj * view * model.transform(rdb); rdb /= rdb.w;
	rub = viewport * proj * view * model.transform(rub); rub /= rub.w;
	ldb = viewport * proj * view * model.transform(ldb); ldb /= ldb.w;
	lub = viewport * proj * view * model.transform(lub); lub /= lub.w;

	DrawLine(rdf, ldf, glm::ivec3(0, 0, 1));
	DrawLine(rdf, rdb, glm::ivec3(0, 0, 1));
	DrawLine(ldf, ldb, glm::ivec3(0, 0, 1));
	DrawLine(rdb, ldb, glm::ivec3(0, 0, 1));

	DrawLine(rdf, ruf, glm::ivec3(0, 0, 1));
	DrawLine(ldf, luf, glm::ivec3(0, 0, 1));
	DrawLine(ldb, lub, glm::ivec3(0, 0, 1));
	DrawLine(rdb, rub, glm::ivec3(0, 0, 1));

	DrawLine(ruf, luf, glm::ivec3(0, 0, 1));
	DrawLine(ruf, rub, glm::ivec3(0, 0, 1));
	DrawLine(luf, lub, glm::ivec3(0, 0, 1));
	DrawLine(rub, lub, glm::ivec3(0, 0, 1));
}


void Renderer::drawNormals(const Scene& scene,Face& face,int faceIndex)
{
	MeshModel model = scene.GetActiveModel();
	glm::mat4x4 view = scene.getActiveCamera().getViewTransformation();
	glm::mat4 viewport = scene.getActiveCamera().view_port;
	glm::mat4 proj = scene.getActiveCamera().projection_transformation;
	
	glm::vec4 vec = glm::vec4(model.getVertexNormal(faceIndex),1);
	glm::vec4 vecNorm = 1.1f * vec;

	vec = viewport * proj * view * model.transform(vec); vec /= vec.w;
	vecNorm = viewport * proj * view * model.transform(vecNorm); vecNorm /= vecNorm.w;
	DrawLine(vec, vecNorm, glm::ivec3(0, 0, 1));


}



void Renderer::Render(const Scene& scene)
{
	// TODO: Replace this code with real scene rendering code


	int half_width = viewport_width / 2;
	int half_height = viewport_height / 2;
		;
	if (scene.GetModelCount() > 0)
	{
		if (scene.showAxis)
		{
			drawAxis(scene);
		}

		if (scene.showBoundingBox)
		{
			drawBoundingBox(scene);
		}

		MeshModel model = scene.GetModel(0);
		for (int i = 0; i < model.GetFacesCount(); i++)
		{
			Face face = model.GetFace(i);

			glm::vec4 v1 = glm::vec4(model.GetVertex(face.GetVertexIndex(0) - 1),1);
			glm::vec4 v2 = glm::vec4( model.GetVertex(face.GetVertexIndex(1) - 1),1);
			glm::vec4 v3 = glm::vec4(model.GetVertex(face.GetVertexIndex(2) - 1),1);
			glm::mat4x4 view = scene.getActiveCamera().getViewTransformation();
			glm::mat4 viewport = scene.getActiveCamera().view_port;
			glm::mat4 proj = scene.getActiveCamera().projection_transformation;


			v1 = viewport * proj * view * (model.transform(v1));
			v2 = viewport * proj * view * (model.transform(v2));
			v3 = viewport * proj * view * (model.transform(v3));


			v1 /= v1.w;
			v2 /= v2.w;
			v3 /= v3.w;
		
			DrawLine(glm::ivec2(v1.x , v1.y ), glm::ivec2(v2.x , v2.y ), glm::ivec3(0, 0, 0));
			DrawLine(glm::ivec2(v2.x , v2.y ), glm::ivec2(v3.x , v3.y ), glm::ivec3(0, 0, 0));
			DrawLine(glm::ivec2(v3.x , v3.y ), glm::ivec2(v1.x , v1.y ), glm::ivec3(0, 0, 0));

			//drawNormals(scene,face,i);


		}

	}
}

int Renderer::GetViewportWidth() const
{
	return viewport_width;
}

int Renderer::GetViewportHeight() const
{
	return viewport_height;
}