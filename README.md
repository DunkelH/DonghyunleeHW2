# HW2
202111337 lee dong hyun computer graphics homework2

# Code Description
# Q1: Phong Shading + 그림자
### 1. Material 구조체
struct Material {
    vec3 ka, kd, ks; // Ambient, Diffuse, Specular 반사 계수
    float specular_power; // shininess 값
};
→ 각 물체가 어떤 빛 반응을 보이는지를 정의.

### 2. HitInfo 구조체
struct HitInfo {
    float t;
    vec3 point;
    vec3 normal;
    Material material;
};
→ 광선이 물체에 닿았을 때의 교차점 정보 저장 (위치, 노멀 벡터, 재질 등).

### 3. Surface 클래스 및 상속
virtual bool intersect(const Ray& ray, HitInfo& hit) const = 0;
→ 기존의 intersect() 함수는 bool만 반환했는데, 이제는 HitInfo에 충돌 정보를 저장함.

Sphere, Plane 클래스는 HitInfo를 채워서 교차점 정보 반환하도록 수정됨.

### 4. Scene::intersect() 함수
bool intersect(const Ray& ray, HitInfo& closestHit) const;
→ 여러 물체 중 가장 가까운 교차점만 반환 (Z-buffer 개념).

5. Scene::isInShadow()
bool isInShadow(const vec3& point, const vec3& lightPos) const;
→ 그림자 레이(Shadow Ray)를 통해 빛과 교차점 사이에 다른 물체가 있는지 확인.

6. phongShading() 함수
vec3 phongShading(const HitInfo& hit, const vec3& lightPos, const vec3& eyePos, const Scene& scene)
→ Phong 모델을 기반으로 ambient, diffuse, specular 성분을 계산:

ambient = ka

diffuse = kd * max(dot(N, L), 0)

specular = ks * (dot(R, V)^n)

그림자일 경우 ambient만 적용

# Q2: 감마 보정
### 1. 감마 보정 수식 적용
float gamma = 2.2f;
color = pow(color, vec3(1.0f / gamma));
→ 모니터의 감마 특성을 보정해서 사람이 보기 좋게 표현. → pow(vec3, float)은 각 RGB 성분에 제곱근처럼 적용.

# Q3: 앤티앨리어싱
### 1. Camera::generateRay() 함수 확장
Ray generateRay(int i, int j, int width, int height, float u_offset, float v_offset);
→ 픽셀 내부에서 서브픽셀 위치를 지정해서 랜덤 샘플링 가능하게 함.

### 2. render() 루프에 N = 64 반복 추가
for (int s = 0; s < N; ++s) {
    float u_offset = random(), v_offset = random();
    Ray ray = camera.generateRay(i, j, Width, Height, u_offset, v_offset);
    ...
}
→ 각 픽셀을 64개의 랜덤 점에서 샘플링해서 aliasing 제거 (박스 필터 적용 효과).

### 3. 샘플 색상 누적 및 평균화
vec3 accumulatedColor(0.0f);
accumulatedColor += pow(color, vec3(1.0f / gamma));
accumulatedColor /= N;
→ 부드러운 경계선을 만들어 aliasing 줄임.
