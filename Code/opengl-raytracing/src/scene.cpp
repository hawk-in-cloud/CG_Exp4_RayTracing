#include <string>

#include "scene.h"

#include <iostream>
#include "gui.h"

extern bool refreshRequired;

namespace Scene {
	GLuint boundShader;
	std::vector<Object> objects;
	std::vector<PointLight> lights;
	Material planeMaterial;

	GLuint skyboxTexture;

	glm::vec3 cameraPosition(0, 1, 2);
	float cameraYaw = 0.0f, cameraPitch = 0.0f;

	int shadowResolution = 20;
	int lightBounces = 5;
	int framePasses = 4;
	float blur = 0.002f; // Slight blur (les than a pixel) = anti-aliasing
	float bloomRadius = 0.02f;
	float bloomIntensity = 0.5f;
	float skyboxStrength = 1.0F;
	float skyboxGamma = 2.2F;
	float skyboxCeiling = 10.0F;
	bool planeVisible = true;

	int selectedObjectIndex = -1;

	Material::Material() = default;

	Material::Material(const std::initializer_list<float>& albedo) : Material::Material(albedo, {0,0,0}, {0,0,0}, 1.0f, 1.0f, 0.0f, 0.5f) {}

	Material::Material(const std::initializer_list<float>& albedo, const std::initializer_list<float>& specular, const std::initializer_list<float>& emission, float emissionStrength, float roughness, float specularHighlight, float specularExponent) {
		for (int i = 0; i < 3; i++) {
			this->albedo[i] = *(albedo.begin() + i);
			this->specular[i] = *(specular.begin() + i);
			this->emission[i] = *(emission.begin() + i);
		}
		this->emissionStrength = emissionStrength;
		this->roughness = roughness;
		this->specularHighlight = specularHighlight;
		this->specularExponent = specularExponent;
	}

	Object::Object() = default;

	Object::Object(unsigned int type, const std::initializer_list<float>& position, const std::initializer_list<float>& scale, Material material) {
		this->type = type;
		for (int i = 0; i < 3; i++) this->position[i] = *(position.begin()+i);
		for (int i = 0; i < 3; i++) this->scale[i] = *(scale.begin() + i);
		this->material = material;
	}

	PointLight::PointLight() = default;
	PointLight::PointLight(const std::initializer_list<float>& position, float radius, const std::initializer_list<float>& color, float power, float reach) {
		for (int i = 0; i < 3; i++) this->position[i] = *(position.begin() + i);
		this->radius = radius;
		for (int i = 0; i < 3; i++) this->color[i] = *(color.begin() + i);
		this->power = power;
		this->reach = reach;
	}

	void placeMirrorSpheres() {

	}

	void sendObjectData(int objectIndex) {
		std::string i_str = std::to_string(objectIndex);
		glUniform1ui(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].type").c_str()), objects[objectIndex].type);
		glUniform3f(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].position").c_str()), objects[objectIndex].position[0], objects[objectIndex].position[1], objects[objectIndex].position[2]);
		glUniform3f(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].scale").c_str()), objects[objectIndex].scale[0], objects[objectIndex].scale[1], objects[objectIndex].scale[2]);
		glUniform3f(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].material.albedo").c_str()), objects[objectIndex].material.albedo[0], objects[objectIndex].material.albedo[1], objects[objectIndex].material.albedo[2]);
		glUniform3f(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].material.specular").c_str()), objects[objectIndex].material.specular[0], objects[objectIndex].material.specular[1], objects[objectIndex].material.specular[2]);
		glUniform3f(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].material.emission").c_str()), objects[objectIndex].material.emission[0], objects[objectIndex].material.emission[1], objects[objectIndex].material.emission[2]);
		glUniform1f(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].material.emissionStrength").c_str()), objects[objectIndex].material.emissionStrength);
		glUniform1f(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].material.roughness").c_str()), objects[objectIndex].material.roughness);
		glUniform1f(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].material.specularHighlight").c_str()), objects[objectIndex].material.specularHighlight);
		glUniform1f(glGetUniformLocation(boundShader, std::string("u_objects[").append(i_str).append("].material.specularExponent").c_str()), objects[objectIndex].material.specularExponent);
	}

	void bind(GLuint shaderProgram) {
		boundShader = shaderProgram;

		for (int i = 0; i < lights.size(); i++) {
			glUniform3f(glGetUniformLocation(shaderProgram, std::string("u_lights[").append(std::to_string(i)).append("].position").c_str()), lights[i].position[0], lights[i].position[1], lights[i].position[2]);
			glUniform1f(glGetUniformLocation(shaderProgram, std::string("u_lights[").append(std::to_string(i)).append("].radius").c_str()), lights[i].radius);
			glUniform3f(glGetUniformLocation(shaderProgram, std::string("u_lights[").append(std::to_string(i)).append("].color").c_str()), lights[i].color[0], lights[i].color[1], lights[i].color[2]);
			glUniform1f(glGetUniformLocation(shaderProgram, std::string("u_lights[").append(std::to_string(i)).append("].power").c_str()), lights[i].power);
			glUniform1f(glGetUniformLocation(shaderProgram, std::string("u_lights[").append(std::to_string(i)).append("].reach").c_str()), lights[i].reach);
		}

		glUniform3f(glGetUniformLocation(shaderProgram, "u_planeMaterial.albedo"), planeMaterial.albedo[0], planeMaterial.albedo[1], planeMaterial.albedo[2]);
		glUniform3f(glGetUniformLocation(shaderProgram, "u_planeMaterial.specular"), planeMaterial.specular[0], planeMaterial.specular[1], planeMaterial.specular[2]);
		glUniform3f(glGetUniformLocation(shaderProgram, "u_planeMaterial.emission"), planeMaterial.emission[0], planeMaterial.emission[1], planeMaterial.emission[2]);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_planeMaterial.emissionStrength"), planeMaterial.emissionStrength);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_planeMaterial.roughness"), planeMaterial.roughness);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_planeMaterial.specularHighlight"), planeMaterial.specularHighlight);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_planeMaterial.specularExponent"), planeMaterial.specularExponent);

		glUniform1i(glGetUniformLocation(shaderProgram, "u_shadowResolution"), shadowResolution);
		glUniform1i(glGetUniformLocation(shaderProgram, "u_lightBounces"), lightBounces);
		glUniform1i(glGetUniformLocation(shaderProgram, "u_framePasses"), framePasses);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_blur"), blur);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_bloomRadius"), bloomRadius);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_bloomIntensity"), bloomIntensity);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_skyboxStrength"), skyboxStrength);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_skyboxGamma"), skyboxGamma);
		glUniform1f(glGetUniformLocation(shaderProgram, "u_skyboxCeiling"), skyboxCeiling);

		for (int i = 0; i < objects.size(); i++) {
			sendObjectData(i);
		}

		glUniform1i(glGetUniformLocation(boundShader, "u_selectedSphereIndex"), selectedObjectIndex);
		glUniform1i(glGetUniformLocation(boundShader, "u_planeVisible"), planeVisible);
	}

	void unbind() {
		boundShader = 0;
	}

	bool sphereIntersection(glm::vec3 position, float radius, glm::vec3 rayOrigin, glm::vec3 rayDirection, float* hitDistance) {
		// 计算射线与球心的向量投影长度，t表示射线起点到交点的距离（假设射线与球心连线的方向是正确的）
		float t = glm::dot(position - rayOrigin, rayDirection);

		// 计算射线在 t 距离处的点 p
		glm::vec3 p = rayOrigin + rayDirection * t;

		// 计算从点 p 到球心的距离
		float y = glm::length(position - p);

		// 如果点 p 到球心的距离小于球的半径，说明射线与球相交
		if (y < radius) {
			// 计算射线与球交点的另一个交点的距离
			float x = sqrt(radius * radius - y * y);

			// 计算第一个交点的 t 值（即距离射线起点的距离）
			float t1 = t - x;

			// 如果交点 t1 在射线的前进方向上（t1 > 0），说明交点有效
			if (t1 > 0) {
				*hitDistance = t1;  // 更新最近交点的距离
				return true;  // 返回 true 表示有交点
			}
		}

		// 如果没有交点，则返回 false
		return false;
	}


	bool boxIntersection(glm::vec3 position, glm::vec3 size, glm::vec3 rayOrigin, glm::vec3 rayDirection, float* hitDistance) {
		float t1 = -1000000000000.0;
		float t2 = 1000000000000.0;

		glm::vec3 boxMin = position - size / 2.0f;
		glm::vec3 boxMax = position + size / 2.0f;

		glm::vec3 t0s = (boxMin - rayOrigin) / rayDirection;
		glm::vec3 t1s = (boxMax - rayOrigin) / rayDirection;

		glm::vec3 tsmaller = min(t0s, t1s);
		glm::vec3 tbigger = max(t0s, t1s);

		t1 = std::max({ t1, tsmaller.x, tsmaller.y, tsmaller.z });
		t2 = std::min({ t2, tbigger.x, tbigger.y, tbigger.z });

		*hitDistance = t1;

		return t1 >= 0 && t1 <= t2;
	}

	bool planeIntersection(glm::vec3 planeNormal, glm::vec3 planePoint, glm::vec3 rayOrigin, glm::vec3 rayDirection, float* hitDistance)
	{
		float denom = glm::dot(planeNormal, rayDirection);
		if (abs(denom) > 0.0001) {
			glm::vec3 d = planePoint - rayOrigin;
			*hitDistance = glm::dot(d, planeNormal) / denom;
			return (*hitDistance >= 0.0001);
		}

		return false;
	}

	void selectHovered(float mouseX, float mouseY, int screenWidth, int screenHeight, glm::vec3 cameraPosition, glm::mat4 rotationMatrix) {
		float relativeMouseX = mouseX / screenWidth;
		float relativeMouseY = 1.0 - mouseY / screenHeight;
		glm::vec2 centeredUV = (2.0f * glm::vec2(relativeMouseX, relativeMouseY) - glm::vec2(1.0)) * glm::vec2((float)screenWidth / screenHeight, 1.0);
		glm::vec3 rayDir = glm::normalize(glm::vec4(centeredUV, -1.0, 0.0)) * rotationMatrix;

		float minDist = -1;
		selectedObjectIndex = -1;
		for (int sphereIndex = 0; sphereIndex < objects.size(); sphereIndex++) {
			if (objects[sphereIndex].type == 0) continue;

			float dist;
			if (objects[sphereIndex].type == 1 && sphereIntersection(glm::vec3(objects[sphereIndex].position[0], objects[sphereIndex].position[1], objects[sphereIndex].position[2]), objects[sphereIndex].scale[0], cameraPosition, rayDir, &dist)) {
				if (minDist == -1 || dist < minDist) {
					minDist = dist;
					selectedObjectIndex = sphereIndex;
				}
			}
			else if (objects[sphereIndex].type == 2 && boxIntersection(glm::vec3(objects[sphereIndex].position[0], objects[sphereIndex].position[1], objects[sphereIndex].position[2]), glm::vec3(objects[sphereIndex].scale[0], objects[sphereIndex].scale[1], objects[sphereIndex].scale[2]), cameraPosition, rayDir, &dist)) {
				if (minDist == -1 || dist < minDist) {
					minDist = dist;
					selectedObjectIndex = sphereIndex;
				}
			}
		}

		if (boundShader) {
			glUniform1i(glGetUniformLocation(boundShader, "u_selectedSphereIndex"), selectedObjectIndex);
		}
	}

	void mousePlace(float mouseX, float mouseY, int screenWidth, int screenHeight, glm::vec3 cameraPosition, glm::mat4 rotationMatrix) {
		// 将鼠标坐标转为相对屏幕坐标
		float relativeMouseX = mouseX / screenWidth;
		float relativeMouseY = 1.0 - mouseY / screenHeight;

		// 转换为归一化设备坐标（NDC）
		glm::vec2 centeredUV = (2.0f * glm::vec2(relativeMouseX, relativeMouseY) - glm::vec2(1.0)) * glm::vec2((float)screenWidth / screenHeight, 1.0);

		// 计算射线方向
		glm::vec3 rayDir = glm::normalize(glm::vec4(centeredUV, -1.0, 0.0)) * rotationMatrix;

		glm::vec3 planeNormal = glm::vec3(0, 1, 0);  // 平面的法线方向
		float denom = glm::dot(planeNormal, rayDir);  // 计算射线与平面的点积

		if (abs(denom) > 0.0001) {  // 判断射线是否与平面平行
			glm::vec3 d = -cameraPosition;
			float hitDistance = glm::dot(d, planeNormal) / denom;  // 计算射线与平面的交点

			if (hitDistance >= 0.0001) {  // 如果交点有效
				glm::vec3 position = cameraPosition + rayDir * hitDistance;  // 计算交点位置

				// 如果有选中的物体，则在交点位置放置物体
				if (selectedObjectIndex >= 0) {
					Object selectedObject = objects[selectedObjectIndex];
					objects.push_back(Object(
						selectedObject.type,
						{ position[0], position[1] + (selectedObject.type == 1 ? selectedObject.scale[0] : (selectedObject.scale[1] / 2.0f)), position[2] },
						{ selectedObject.scale[0], selectedObject.scale[1], selectedObject.scale[2] },
						selectedObject.material
					));
				}
				// 如果没有选中的物体，放置一个默认物体
				else {
					objects.push_back(Object(
						1,
						{ position[0], position[1] + 1.0f, position[2] },
						{ 1.0f, 1.0f, 1.0f },
						Material({ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f, 1.0f, 0.0f, 0.0f)
					));
				}

				sendObjectData(objects.size() - 1);  // 发送新物体数据
				refreshRequired = true;  // 标记刷新
			}
		}
	}


}