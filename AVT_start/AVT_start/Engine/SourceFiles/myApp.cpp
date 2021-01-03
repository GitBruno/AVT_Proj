#include "..\HeaderFiles\myApp.h"

void myApp::processInput(GLFWwindow* win, double elapsed)
{
	Camera* cam = graph.getCam();

	if (reset_cam) {
		cam->eye = Vector3D(0, 0, 20);
		cam->center = Vector3D(0, 0, 0);
		cam->up = Vector3D(0, 1, 0);

		cam->updateView();
		reset_cam = false;
	}
	else {
		walk(win, elapsed);
		look(win, elapsed);
	}

	animate(win, elapsed);
}

void myApp::animate(GLFWwindow* win, double elapsed)
{
	t_frame += elapsed;

	graph.setTransform("cube", MxFactory::rotation4(Vector3D(0,1,0), t_frame * 90));
}

void myApp::look(GLFWwindow* win, double elapsed)
{
	Camera* cam = graph.getCam();

	double x, y;
	glfwGetCursorPos(win, &x, &y);

	int w, h;
	glfwGetWindowSize(win, &w, &h);

	double move_x = (x - old_x);
	double move_y = (y - old_y);

	if (move_x != 0 || move_y != 0)
		cam->look(angle_x * move_x * elapsed, angle_y * move_y * elapsed);

	old_x = x;
	old_y = y;
}

void myApp::walk(GLFWwindow* win, double elapsed)
{
	Camera* cam = graph.getCam();
	int r = (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS),
		l = (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS),
		d = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS),
		u = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);

	double xaxis = (double)r - l,
		yaxis = (double)d - u;

	if (xaxis != 0 || yaxis != 0) {
		Vector3D dir = Vector3D(xaxis, 0, yaxis);
		dir.normalize();

		cam->move(dir, sprint_factor * speed * elapsed);
	}
}

void myApp::save(GLFWwindow* win)
{
	int w, h;
	glfwGetWindowSize(win, &w, &h);

	GLubyte* pixels = new GLubyte[3 * w * h];

	glReadPixels(0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, pixels);

	// Convert to FreeImage format & save to file
	FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, w, h, 3 * w, 24, 0x0000FF, 0xFF0000, 0x00FF00, false);
	FreeImage_Save(FIF_BMP, image, "test.bmp", 0);

	// Free resources
	FreeImage_Unload(image);
	delete[] pixels;

	//std::cout << "width: " << w << "; height: " << h << std::endl;

}

void myApp::populateScene()
{
	// Camera init
	Camera* cam = new Camera(Vector3D(8, 8, 8), Vector3D(0, 0, 0), Vector3D(0, 1, 0));
	cam->perspectiveProjection(60, 4.0f / 3.0f, 1, 200);
	cam->init();

	graph.setCamera(cam);
	
	// Outline
	Shader* outline_shader = new Shader("res/shaders/outline_vs.glsl", "res/shaders/outline_fs.glsl");
	outline_shader->addUniformBlock("Matrices", 0);
	Material* outline_material = new Material(outline_shader);
	graph.setOutline(outline_material);

	//Asset Manager
	Manager* h = Manager::getInstance();

	// Meshes
	Mesh* cube_mesh = h->addMesh("cube_mesh", new Mesh("res/meshes/bunny_smooth.obj"));
	Mesh* tous_mesh = h->addMesh("torus_mesh", new Mesh("res/meshes/torus.obj"));

	// Shaders
	Shader* blinn_phong_shader = h->addShader("blinn_phong_shader", new Shader("res/shaders/blinn_phong_vs.glsl", "res/shaders/blinn_phong_fs.glsl"));
	blinn_phong_shader->addUniformBlock("Matrices", 0);

	// Materials 
	Material* test_mat_g = h->addMaterial("test_mat_g", new Material(blinn_phong_shader));
	test_mat_g->setUniformVec3("u_AlbedoColor", Vector3D(0, 1, 0));

	//cube
	graph.addChild(test_mat_g, cube_mesh, "cube");

	//cube
	graph.addChild(test_mat_g, tous_mesh, "torus", MxFactory::translation4(Vector3D(0,0,-5)));
}

void myApp::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
	Camera* cam = graph.getCam();
	std::cout << "key: " << key << " " << scancode << " " << action << " " << mods << std::endl;
	if (action == GLFW_RELEASE) {
		switch (key) {
		case GLFW_KEY_P:
			if (cam->projType == CameraProj::Parallel) {
				cam->perspectiveProjection(60, 4.0f / 3.0f, 1, 50);
			}
			else {
				cam->parallelProjection(-10, 10, -10, 10, 1, 50);
			}
			break;
		case GLFW_KEY_ESCAPE:
			if (cam->state == Working::On) {
				glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
			else {
				glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			cam->toggle();
			break;
		case GLFW_KEY_LEFT_SHIFT:
			sprint_factor = 1;
			break;
		case GLFW_KEY_F:
			animate_frame = true;
			break;
		case GLFW_KEY_R:
			reset_cam = true;
			break;
		case GLFW_KEY_I:
			save_img = true;
			break;
		case GLFW_KEY_C:
			set_child = true;
			break;
		case GLFW_KEY_M:
			add_mesh = true;
			break;
		case GLFW_KEY_N:
			new_mat = true;
			break;
		}
	}
	else if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_LEFT_SHIFT:
			sprint_factor = 3;
			break;
		}
	}
	if (key == GLFW_KEY_LEFT) //left arrow - object movement
	{
		MxFactory m = MxFactory();
		Vector3D axis = { 0, 1, 0 };
		Matrix4 rot = m.rotation4(axis, 1);
		SceneNode* root = graph.getNode("root");
		SceneNode* n;
		if (root->selected) {
			graph.applyTransform("root", rot);
		}
		/*for (int i = 0; i < root->children.size(); i++) {
			n = root->children[i];
			if (n->selected) {
				string name = n->getName();
				graph.applyTransform(name, rot);
			}
		}*/
		//rotateAroundAxis(axis, angle)
	}

	else if (key == GLFW_KEY_RIGHT) //right arrow - object movement
	{
		MxFactory m = MxFactory();
		Vector3D axis = { 0, 1, 0 };
		Matrix4 rot = m.rotation4(axis, -1);
		SceneNode* root = graph.getNode("root");
		SceneNode* n;
		if (root->selected) {
			graph.applyTransform("root", rot);
		}
	}

	else if (key == GLFW_KEY_UP) //up arrow - object movement
	{
		MxFactory m = MxFactory();
		Vector3D axis = { 1, 0, 0 };
		Matrix4 rot = m.rotation4(axis, 1);
		SceneNode* root = graph.getNode("root");
		SceneNode* n;
		if (root->selected) {
			graph.applyTransform("root", rot);
		}
	}

	else if (key == GLFW_KEY_DOWN) //down arrow - object movement
	{
		MxFactory m = MxFactory();
		Vector3D axis = { 1, 0, 0 };
		Matrix4 rot = m.rotation4(axis, -1);
		SceneNode* root = graph.getNode("root");
		SceneNode* n;
		if (root->selected) {
			graph.applyTransform("root", rot);
		}
	}
	else if (key == GLFW_KEY_A && action == GLFW_PRESS) //a key (left) - camera movement
	{
		Vector3D axis = { 0, 1, 0 };
		cam->move(axis, -5);
	}
	else if (key == GLFW_KEY_D && action == GLFW_PRESS) //d key (right)- camera movement
	{
		Vector3D axis = { 0, 1, 0 };
		cam->move(axis, 5);
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS)//s key (down) - camera movement
	{
		Vector3D axis = { 1, 0, 0 };
		cam->move(axis, -5);
	}
	else if (key == GLFW_KEY_W && action == GLFW_PRESS)//w key (up) - camera movement
	{
		Vector3D axis = { 1, 0, 0 };
		cam->move(axis, 5);
	}
	else if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
	{
		//graph.serializeScene(&C);
		cout << "serialize\n";
	}
	else if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		cout << "deserialize\n";
		//graph.deserializeScene();
	}
}

void myApp::update(GLFWwindow *win, double elapsed)
{
	processInput(win, elapsed);
	graph.draw();
	if (save_img) {
		save(win);
		save_img = false;
	}
	if (set_child) {
		graph.setTransform("torus", MxFactory::invrotation4(Vector3D(0, 1, 0), t_frame * 90) * MxFactory::translation4(Vector3D(0,0,-5)));
		graph.changeParent("torus", "cube");
		set_child = false;
	}
	if (add_mesh) {
		Manager* h = Manager::getInstance();
		Mesh* mesh = h->addMesh("new_mesh", new Mesh("res/meshes/smooth.obj"));
		mesh->init();

		Material* material = h->getMaterial("test_mat_g");

		graph.setCurrToRoot();
		graph.addChild(material, mesh, "new_guy", MxFactory::translation4(Vector3D(0,5,0)));
		add_mesh = false;
	}
	if (new_mat) {
		Manager* h = Manager::getInstance();
		SceneNode* node = graph.getNode("cube");

		Shader* toon_shader = h->addShader("toon_shader", new Shader("res/shaders/toon_vs.glsl", "res/shaders/toon_fs.glsl"));
		toon_shader->addUniformBlock("Matrices", 0);

		Texture* toon_ramp_texture = h->addTexture("toon_ramp_texture", new Texture("res/textures/toon_ramp_texture.png"));

		Material* test_mat_b = h->addMaterial("test_mat_b", new Material(toon_shader));
		test_mat_b->setUniformVec3("u_AlbedoColor", Vector3D(0, 0, 1));
		test_mat_b->setTexture(toon_ramp_texture);

		node->material = test_mat_b;
		new_mat = false;
	}
}
