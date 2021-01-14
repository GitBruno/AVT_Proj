#include "..\HeaderFiles\serializer.h"

SceneSerializer::SceneSerializer(Camera* cam, Manager* man, SceneNode* par) {
	_camera = cam;
	_manager = man;
	_parent = par;
}

SceneSerializer::SceneSerializer(Camera* cam) {
	_camera = cam;
}

SceneSerializer::SceneSerializer(SceneNode* par) {
	_parent = par;
}

SceneSerializer::~SceneSerializer()
{
//todo
}

void SceneSerializer::serializeCamera(std::ofstream& out)
{
	out << "Camera:\n" << "- Type: " << _camera->projTypeToString() << "\n- Eye, Center, Up: " << _camera->eye << ", " << _camera->center << ", " << _camera->up << "\n- Working State: " << _camera->workingStateToString() << "\n- View Matrix: " << _camera->getView().toString() << "\n- Projection Matrix: " << _camera->getProjection().toString() << "\n- InvView Matrix: " << _camera->getInvView().toString() << std::endl;
}

void SceneSerializer::serializeManager(std::ofstream& out)
{
	out << "Manager:\n";

	std::unordered_map<std::string, Shader*> shaders = _manager->getShaders();
	std::unordered_map<std::string, Shader*>::iterator itShaders;
	std::unordered_map<std::string, Mesh*> meshes = _manager->getMeshes();
	std::unordered_map<std::string, Mesh*>::iterator itMeshes;
	std::unordered_map<std::string, Texture*> textures = _manager->getTextures();
	std::unordered_map<std::string, Texture*>::iterator itTextures;

	for (itShaders = shaders.begin(); itShaders != shaders.end(); itShaders++)
	{
		std::string name = itShaders->first;
		out << "- Shader:\n\t- Name: " << name << "\n\t- Vertex Shader Filepath: " << _manager->getShader(name)->vertexShaderFilepath << "\n\t- Fragment Shader Filepath: " << _manager->getShader(name)->fragmentShaderFilepath << std::endl;
	}

	for (itMeshes = meshes.begin(); itMeshes != meshes.end(); itMeshes++)
	{
		std::string name = itMeshes->first;
		out << "- Mesh:\n\t- Name: " << name << "\n\t- Filepath: " << _manager->getMesh(name)->file_path << std::endl;
	}

	for (itTextures = textures.begin(); itTextures != textures.end(); itTextures++)
	{
		std::string name = itTextures->first;
		out << "- Texture:\n\t- Name: " << name << "\n\t- Filepath: " << _manager->getTexture(name)->file_path << std::endl;
	}

	//materials � diferente - todo

}

void SceneSerializer::serializeNode(SceneNode* node, std::ofstream& out)
{
	out << "Node:\n" << "- Name: " << node->getName() << "\n- Children: " << node->children.size() 
		<< "\n- Material:" << "\n\t- Name: " << node->materialName << "\n\t- Texture:\n\t\t- Name: " << node->textureName << "\n\t\t- FilePath: " << node->textureFile
		<< "\n\t- Shaders:\n\t\t- Name: " << node->shaderName << "\n\t\t- Vertex Shader Filepath: " << node->vertexShaderFile << "\n\t\t- Fragment Shader Filepath: " << node->fragmentShaderFile
		<< "\n\t- Outline: " << node->outline
		<< "\n- Mesh: \n\t- Name: "<< node->meshName <<"\n\t- Filepath: " << node->meshFile 
		<< "\n- Transformation: " << node->getTransform().toString() << std::endl;
	SceneNode* n;
	for (int i = 0; i < node->children.size(); i++) {
		n = node->children[i];
		serializeNode(n, out);
	}
}

void SceneSerializer::serialize(const std::string& filepath)
{
	SceneNode* n;
	std::ofstream file(filepath);
	serializeCamera(file);
	serializeManager(file);
	file << "Node:\n" << "- Name: " << _parent->getName() << "\n- Children: " << _parent->children.size() 
		<< "\n- Material:" << "\n\t- Name: " << _parent->materialName << "\n\t- Texture:\n\t\t- Name: " << _parent->textureName << "\n\t\t- FilePath: " << _parent->textureFile
		<< "\n\t- Shaders:\n\t\t- Name: " << _parent->shaderName << "\n\t\t- Vertex Shader Filepath: " << _parent->vertexShaderFile << "\n\t\t- Fragment Shader Filepath: " << _parent->fragmentShaderFile
		<< "\n\t- Outline: " << _parent->outline
		<< "\n- Mesh: \n\t- Name: " << _parent->meshName << "\n\t- Filepath: " << _parent->meshFile 
		<< "\n- Transformation: " << _parent->getTransform().toString() << std::endl;

	for (int i = 0; i < _parent->children.size(); i++) {
		n = _parent->children[i];
		serializeNode(n, file);
	}
	file.close();
}

void SceneSerializer::deserialize(const std::string& filepath)
{
	std::ifstream file(filepath, ios::in);
	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		cout << line << endl;
	}

	file.close();
}