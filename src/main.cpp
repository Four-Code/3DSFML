#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>
#include <functional>

float screenHeight = 800;
float screenWidth = 1200;
FILE* pipe = nullptr;
float distSquare(float x1, float y1, float x2, float y2){
    return pow(x1-x2, 2) + pow(y1 - y2, 2);
}

float mag(sf::Vector3f a){
    return std::sqrt(a.x*a.x + a.y*a.y+ a.z*a.z);
}

sf::Vector3f cap(sf::Vector3f a){
    return {a.x/mag(a), a.y/mag(a), a.z/mag(a)};
}

sf::Vector3f crossProduct(sf::Vector3f a, sf::Vector3f b){
    return {(a.y*b.z - a.z*b.y), -(a.x*b.z - a.z*b.x), (a.x*b.y - a.y*b.x)};
}
sf::Vector3f crossProduct(sf::Vector2f a, sf::Vector2f b){
    return {0, 0, (a.x*b.y - a.y*b.x)};
}
float dotProduct(sf::Vector3f a, sf::Vector3f b){
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

class PhysicsObject{
    public:
    sf::Vector3f acc{0, 0, 0};
    sf::Vector3f vel{0, 0, 0};
    sf::Vector3f pos{0, 0, 0};
    float mass = 1;
    float restitution = 1;
    
    
    void integrate(float dt){
        pos += vel*dt;
        vel += acc*dt;
    }
};

class Point:public PhysicsObject{
    public:
    sf::CircleShape shape;
    Point(sf::Vector3f posPara={0, 0, 0})
    :shape(2)
    {
        pos = posPara;
    }
    void draw(sf::RenderTarget& target, sf::Vector2f screenCoords){
        shape.setPosition(screenCoords);
        target.draw(shape);
    }
};

class Camera:public PhysicsObject{
    public:
    sf::Vector3f direction = {0, 0, 1};
    float scale;
    bool moveForward;
    bool moveBackward;
    bool moveLeft;
    bool moveRight;
    bool moveUp = false;
    bool moveDown = false;
    float horizontalAngle = 0;
    float verticalAngle = 0;
};

class Mouse{
    public:
    sf::Vector2f prevPosition = {0, 0};
    sf::Vector2f currentPosition = {0, 0};
    bool isInitalized = false;
    float sensitivity = 0.0006;
    sf::Vector3f getMovementDirectionVector(){
        return {currentPosition.x - prevPosition.x, currentPosition.y - prevPosition.y, 0};
    }
};

class Line{
    public:
    int point1;
    int point2;
    sf::RectangleShape shape;
    Line(int point1Para, int point2Para)
    :point1(point1Para), point2(point2Para)
    {}
    void draw(sf::RenderTarget& target, sf::Vector2f screenPoint1, sf::Vector2f screenPoint2){
        if (screenPoint1.x >-1000 && screenPoint2.x >-1000){
            shape.setSize({ sqrt(distSquare(screenPoint1.x, screenPoint1.y, screenPoint2.x, screenPoint2.y)), 1});
            shape.setOrigin({0, shape.getSize().y/2});
            shape.setRotation(atan2(screenPoint2.y-screenPoint1.y, screenPoint2.x-screenPoint1.x)*(180.0f/3.14));
            shape.setPosition(screenPoint1);
            target.draw(shape);
        }
    }

};

std::vector<sf::Vector2f> resolvePoints(std::vector<Point>& points, Camera& camera){
        std::vector<sf::Vector2f> screenPoints;
        sf::Vector3f worldUp = {0, 1, 0};
        sf::Vector3f forward = {cos(camera.verticalAngle)*sin(camera.horizontalAngle), sin(camera.verticalAngle), cos(camera.verticalAngle)*cos(camera.horizontalAngle)};
        sf::Vector3f right = -cap(crossProduct(forward, worldUp));
        sf::Vector3f up = cap(crossProduct(forward, right));
        sf::Vector3f rotated;
        for (int i = 0; i < points.size(); i++){
            Point& point = points[i];
            sf::Vector3f relative = point.pos - camera.pos;
            rotated.x = dotProduct(relative, right);
            rotated.y = dotProduct(relative, up);
            rotated.z = dotProduct(relative, forward);

            if (rotated.z > 0.01){
                float x_ = rotated.x/rotated.z;
                float y_ = rotated.y/rotated.z;
                float screenx = (x_ * camera.scale) + screenWidth/2;
                float screeny = (-y_ * camera.scale) + screenHeight/2;
                screenPoints.push_back({screenx, screeny});
            }
            else{
                screenPoints.push_back({-1000, -1000});
            }
        }
        return screenPoints;
}

sf::Vector2f resolvePoints(Point& point, Camera& camera){
    sf::Vector2f screenPoint;
    sf::Vector3f worldUp = {0, 1, 0};
    sf::Vector3f forward = {cos(camera.verticalAngle)*sin(camera.horizontalAngle), sin(camera.verticalAngle), cos(camera.verticalAngle)*cos(camera.horizontalAngle)};
    sf::Vector3f right = -cap(crossProduct(forward, worldUp));
    sf::Vector3f up = cap(crossProduct(forward, right));
    sf::Vector3f rotated;

    sf::Vector3f relative = point.pos - camera.pos;
    rotated.x = dotProduct(relative, right);
    rotated.y = dotProduct(relative, up);
    rotated.z = dotProduct(relative, forward);
    if (rotated.z > 0.01){
        float x_ = rotated.x/rotated.z;
        float y_ = rotated.y/rotated.z;
        float screenx = (x_ * camera.scale) + screenWidth/2;
        float screeny = (-y_ * camera.scale) + screenHeight/2;
        screenPoint = {screenx, screeny};
    }
    else{
        screenPoint = {-1000, -1000};
    }
    
    return screenPoint;
}

float lineEquation(sf::Vector2f point1, sf::Vector2f point2, float x, float y){
    // float a = point2.y - point1.y;
    // float b = point2.x - point1.x;
    // float c = point1.x*point2.y - point2.x*point1.y;
    if(point2.x == point1.x){
        return x- point1.x;
    }
    else{
        return y - point1.y - ((point2.y-point1.y)/(point2.x- point1.x))*(x-point1.x);
    }
    // return a*x + b*y + c;
}

bool liesInsideTriangle(sf::Vector2f vertex1, sf::Vector2f vertex2, sf::Vector2f vertex3, float x, float y){
    bool firstlinecheck = false;
    bool secondlinecheck = false;
    bool thirdlinecheck = false;

    //firstline
    if (lineEquation(vertex1, vertex2, vertex3.x, vertex3.y)>0 && lineEquation(vertex1, vertex2, x, y)>0){
        firstlinecheck = true;
    }
    else if (lineEquation(vertex1, vertex2, vertex3.x, vertex3.y)<0 && lineEquation(vertex1, vertex2, x, y)<0){
        firstlinecheck = true;
    }
    else{
        firstlinecheck = false;
    }

    //secondline
    if (lineEquation(vertex2, vertex3, vertex1.x, vertex1.y)>0 && lineEquation(vertex2, vertex3, x, y)>0){
        secondlinecheck = true;
    }
    else if (lineEquation(vertex2, vertex3, vertex1.x, vertex1.y)<0 && lineEquation(vertex2, vertex3, x, y)<0){
        secondlinecheck = true;
    }
    else{
        secondlinecheck = false;
    }

    //thirdline
    if (lineEquation(vertex3, vertex1, vertex2.x, vertex2.y)>0 && lineEquation(vertex3, vertex1, x, y)>0){
        thirdlinecheck = true;
    }
    else if (lineEquation(vertex3, vertex1, vertex2.x, vertex2.y)<0 && lineEquation(vertex3, vertex1, x, y)<0){
        thirdlinecheck = true;
    }
    else{
        thirdlinecheck = false;
    }

    if (firstlinecheck && secondlinecheck && thirdlinecheck){
        return true;
    }
    else{
        return false;
    }
}

float slope(sf::Vector2f a, sf::Vector2f b){
    return ((b.y-a.y)/(b.x- a.x));
}

bool liesOnEdgeofTriangle(sf::Vector2f vertex1, sf::Vector2f vertex2, sf::Vector2f vertex3, float x, float y){
    float edgethickness = 3;
    float distance = std::min({abs(lineEquation(vertex1, vertex2, x, y)), abs(lineEquation(vertex2, vertex3, x, y)), abs(lineEquation(vertex1, vertex3, x, y))});
    // float distance1 = abs(lineEquation(vertex1, vertex2, x, y));
    // float distance2 = abs(lineEquation(vertex2, vertex3, x, y));
    // float distance3 = abs(lineEquation(vertex3, vertex1, x, y));
    if (distance < 2){
        return true;
    }
    else{
        return false;
    }
}

class Triangle{
    public:
    Point vertex1;
    Point vertex2;
    Point vertex3;
    sf::RectangleShape pixel;
    sf::Color color;
    // sf::RectangleShape bound;

    Triangle(Point vertex1para, Point vertex2para, Point vertex3para, sf::Color colorPara)
    :vertex1(vertex1para), vertex2(vertex2para), vertex3(vertex3para), pixel({1, 1}), color(colorPara)
    {
        // pixel.setFillColor(color);
        // bound.setFillColor(sf::Color::White);
    }
    void draw(std::vector<sf::Uint8>& windowPixels, Camera& camera, std::vector<float> & zbuffer){
        sf::Vector2f vertex1R = resolvePoints(vertex1, camera);
        sf::Vector2f vertex2R = resolvePoints(vertex2, camera);
        sf::Vector2f vertex3R = resolvePoints(vertex3, camera);
        sf::Vector3f forward = {cos(camera.verticalAngle)*sin(camera.horizontalAngle), sin(camera.verticalAngle), cos(camera.verticalAngle)*cos(camera.horizontalAngle)};
        sf::Vector3f relative1 = vertex1.pos - camera.pos;
        sf::Vector3f relative2 = vertex2.pos - camera.pos;
        sf::Vector3f relative3 = vertex3.pos - camera.pos;
        float rotated1z = dotProduct(relative1, forward);
        float rotated2z = dotProduct(relative2, forward);
        float rotated3z = dotProduct(relative3, forward);
        int zindex = (rotated1z + rotated2z + rotated3z)/3;
        sf::Vector3f normal = crossProduct(vertex2.pos- vertex1.pos, vertex2.pos- vertex3.pos);
        sf::Vector3f viewvector = camera.pos - (vertex1.pos + vertex2.pos + vertex3.pos)/3.0f;

        if (dotProduct(viewvector, normal)<0){
                
            if (vertex1R.x > -900 && vertex2R.x >-900 && vertex3R.x >-900 && rotated1z > 2.0f &&rotated2z > 2.0f && rotated3z >2.0f){
                
                float minx = std::min({vertex1R.x, vertex2R.x, vertex3R.x});
                float maxx = std::max({vertex1R.x, vertex2R.x, vertex3R.x});
                float miny = std::min({vertex1R.y, vertex2R.y, vertex3R.y});
                float maxy = std::max({vertex1R.y, vertex2R.y, vertex3R.y});
                
                minx = std::max({minx, 0.0f});
                maxx = std::min({maxx, screenWidth-1});
                miny = std::max({miny, 0.0f});
                maxy = std::min({maxy, screenHeight-1});

                sf::Vector3f lightDir = (sf::Vector3f(4,-5,8));
                // std::cout<<lightDir.x<<std::endl;
                // std::cout<<lightDir.y<<std::endl;
                // std::cout<<lightDir.z<<std::endl;
                // std::cout<<std::endl;

                float brightness = (dotProduct(cap(normal), cap(lightDir)));
                brightness = std::max(brightness, 0.2f);
                
                
                for (int y = miny; y < maxy; y++){
                    for (int x = minx; x < maxx; x++){
                        if(liesInsideTriangle(vertex1R, vertex2R, vertex3R, x, y)){
                            // pixel.setPosition({x, y});
                            if (zbuffer[x + y*screenWidth]>=zindex){
                                zbuffer[x + y*screenWidth] = zindex;
                                // if (liesOnEdgeofTriangle(vertex1R, vertex2R, vertex3R, x, y)){
                                //     sf::Color temp = pixel.getFillColor();
                                //     // pixel.setFillColor(sf::Color::White);
                                //     int index = (x + y * screenWidth) * 4;
                                //     windowPixels[index + 0] = 255;
                                //     windowPixels[index + 1] = 255;
                                //     windowPixels[index + 2] = 255;
                                //     windowPixels[index + 3] = 255;
                                //     // pixel.setFillColor(temp);
                                // }
                                // else{
                                    int index = (x + y * screenWidth) * 4;
                                    // windowPixels[index + 0] = color.r*brightness;
                                    // windowPixels[index + 1] = color.g*brightness;
                                    // windowPixels[index + 2] = color.b*brightness;
                                    // windowPixels[index + 3] = color.a;
                                    windowPixels[index + 0] = 255*brightness;
                                    windowPixels[index + 1] = 255*brightness;
                                    windowPixels[index + 2] = 255*brightness;
                                    windowPixels[index + 3] = 255;
                                // }
                            }
                        
                        }
                    }
                
                }
            
            
            }
        }
    }
};

class Cube:public PhysicsObject{
    public:
    std::vector<Point> points{8};
    float side;
    std::vector<sf::Vector3f> pointOffsets;
    // sf::ConvexShape face1;
    // sf::ConvexShape face2;
    // sf::ConvexShape face3;
    // sf::ConvexShape face4;
    // sf::ConvexShape face5;
    // sf::ConvexShape face6;

    std::vector<Triangle> faces;

    // std::vector<std::vector<int>> faceIndexes = {
    //     {0, 1, 2, 3}, {7, 6, 5, 4}, {0, 4, 5, 1}, {3, 2, 6, 7}, {4, 0, 3, 7}, {1, 5, 6, 2}
    // };

    std::vector<std::vector<int>> faceIndexes = {
        {0, 2, 3}, {1, 2, 0}, {0, 3, 7}, {0, 7, 4}, {4, 5, 1}, {4, 1, 0}, {3, 2, 7}, {2, 6, 7}, {1, 6, 2}, {5, 6, 1}, {4, 7, 6}, {5, 4, 6,}
    };

    
    std::vector<Line> lines{
        {0, 1}, {0, 3}, {0, 4},
        {1, 2}, {1, 5},
        {2, 3}, {2, 6},
        {3, 7},
        {4, 5}, {4, 7}, 
        {5, 6}, 
        {6, 7}
    };
    Cube(sf::Vector3f positionPara, float sidePara)
    :side(sidePara),
    pointOffsets{
        {-side/2, side/2, -side/2}, {side/2, side/2, -side/2}, {side/2, -side/2, -side/2}, {-side/2, -side/2, -side/2},
        {-side/2, side/2, side/2}, {side/2, side/2, side/2}, {side/2, -side/2, side/2}, {-side/2, -side/2, side/2}
    }
    {
        pos = positionPara;
        Triangle face({}, {}, {}, sf::Color(77, 77, 77));
        faces.assign(12, face);
    }
    void draw(std::vector<sf::Uint8>& windowPixels, Camera& camera, std::vector<float>&zbuffer){        
        for (int i = 0; i < points.size(); i++){
            points[i].pos = pointOffsets[i]+pos;
        }
        std::vector<sf::Vector2f> screenPoints = resolvePoints(points, camera);


        for (int i = 0; i < faces.size(); i++)
        {
            std::vector<int> index = faceIndexes[i];
            faces[i].vertex1 = points[index[0]];
            faces[i].vertex2 = points[index[1]];
            faces[i].vertex3 = points[index[2]];

        }

        for (int i = 0; i < faces.size(); i++){           
            faces[i].draw(windowPixels, camera, zbuffer);
        }
    }
};

class Circle:public PhysicsObject{
    public:
    float radius;
    float resolution;
    std::vector<Point> points;
    std::vector<sf::Vector3f> pointOffsets;
    std::vector<Line> lines;
    
    Circle(float radiusPara, float resoultionPara = 5)
    :radius(radiusPara), resolution(resoultionPara)
    {
        float theta = 2*3.14/resolution;
        
        for (int i = 0; i < resolution; i++){
            pointOffsets.push_back({radius* cos(i*theta), 0, radius* sin(i*theta)});
            if (i+1<resolution){
                lines.push_back({i, i+1});
            }
            else{
                lines.push_back({i, 0});
            }
        }
        points.resize(pointOffsets.size());
        
    }
    std::vector<sf::Vector2f> resolve(Camera& camera){
        for (int i = 0; i < points.size(); i++){
            points[i].pos = pos+pointOffsets[i];
        }
        
        std::vector<sf::Vector2f> screenPoints = resolvePoints(points, camera);
        return screenPoints;
    }

    void draw(sf::RenderTarget& target, Camera& camera){
        std::vector<sf::Vector2f> screenPoints= resolve(camera);
        for (int i = 0; i < lines.size(); i++){
            lines[i].draw(target, screenPoints[lines[i].point1], screenPoints[lines[i].point2]);
        }        
    }
};

class Sphere:public PhysicsObject{
    public:
    float radius;
    std::vector<Circle> circles;
    std::vector<Line> lines;
    Sphere(float raidusPara, int resolutionPara = 5)
    :radius(raidusPara), resolution(resolutionPara)
    {
        if (resolution%2==0){resolution++;}
        float mid = (resolution+1)/2.0f;
        for (int i = 0; i < resolution; i++){
            float radius_ = sqrt(pow(radius, 2)- pow(((mid-(i+1))*(2*radius/(resolution+1))), 2));
            float yoffset = (mid- (i+1))*(2*radius/(resolution+1));
            Circle circle(radius_, resolution);
            circle.pos = {pos.x, pos.y+yoffset, pos.z};
            circles.push_back(circle);
        }
        
    }
    
    void draw(sf::RenderTarget& target, Camera& camera){
        float mid = (resolution+1)/2;
        resolvedPoints.clear();
        Point bottom({pos.x, pos.y-radius, pos.z});
        Point top({pos.x, pos.y+radius, pos.z});
        std::vector<Point> endPoints = {top, bottom};
        std::vector<sf::Vector2f> resolvedEndPoints = resolvePoints(endPoints, camera);

        for (int i = 0; i < resolution; i++){
            float yoffset = (mid- (i+1))*(2*radius/(resolution+1));
            circles[i].pos = {pos.x, pos.y+yoffset, pos.z};
            resolvedPoints.push_back(circles[i].resolve(camera));
        }

        for (int i = 0; i < resolvedPoints.size(); i++){
            for (int j = 0; j < resolvedPoints[i].size(); j++){
                Line line(j, j);
                Line lineLongitude(j, j+1);
                if (i==0){
                    line.draw(target, resolvedEndPoints[0], resolvedPoints[i][j]);
                }
                if (j+1<resolvedPoints[i].size()){
                    lineLongitude.draw(target, resolvedPoints[i][j], resolvedPoints[i][j+1]);
                }
                else{
                    lineLongitude.draw(target, resolvedPoints[i][j], resolvedPoints[i][0]);
                }
                if (i+1<resolvedPoints.size()){
                    line.draw(target, resolvedPoints[i][j], resolvedPoints[i+1][j]);
                }
                else{
                    line.draw(target, resolvedPoints[i][j], resolvedEndPoints[1]);
                }
            }
        }
        
    }
    private:
    int resolution;
    std::vector<std::vector<sf::Vector2f>> resolvedPoints;
};



int main(){
    sf::ContextSettings settings;
    settings.antialiasingLevel = 6;
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "3D", sf::Style::Fullscreen, settings);
    screenWidth = desktop.width;
    screenHeight = desktop.height;


    window.setMouseCursorGrabbed(true);
    window.setMouseCursorVisible(false);
    window.setFramerateLimit(60);
    sf::Texture texture;
    texture.create(screenWidth, screenHeight);

    sf::Texture windowRenderTexture;
    windowRenderTexture.create(screenWidth, screenHeight);
    sf::Sprite windowSprite;
    windowSprite.setTexture(windowRenderTexture);

    std::string command = "ffmpeg -y -f rawvideo -pixel_format rgba -video_size " +std::to_string((int)screenWidth)+"x" + std::to_string((int)screenHeight) +" -framerate 60 -i - -c:v libx264 -preset ultrafast -pix_fmt yuv420p output.mp4";

    std::vector<sf::Uint8>windowPixels(screenHeight*screenHeight*4);
        

    Camera camera;
    camera.pos = {0, 0, 0};
    camera.scale = 600;
    camera.vel = {0, 0, 0};
    camera.horizontalAngle = 0;

    Mouse mouse;

    std::vector<Cube> cubes;

    std::vector<Sphere> spheres;

    std::vector<float> zbuffer;
    zbuffer.assign(screenWidth*screenHeight, 9999);


    // Sphere sphere(20, 15);
    // sphere.pos = {10, 100, 100};

    // for (int i = 0; i < 20; i++){
    //     for (int j = 0; j < 20; j++){
    //         Cube cube({j*20, 0, i*20},20);
    //         cubes.push_back(cube);
    //     }
        
    // }

    // for (int i = 0; i < 10; i++){
    //     for (int j = 0; j < 10; j++){
    //         Sphere sphere(20, 11);
    //         sphere.pos = {j*80, 300, i*80};
    //         spheres.push_back(sphere);
    //     }
        
    // }
    // Sphere sphere(10, 21);
    // sphere.pos = {50, 100, 50};
    // spheres.push_back(sphere);

    Cube cube({10, 60, 100}, 20);
    cubes.push_back(cube);

    Cube cube1({10, 40, 100}, 20);
    cubes.push_back(cube1);

    Point point1({10, 10, 100});
    Point point2({20, 10, 100});
    Point point3({20, 20, 100});
    Triangle tri(point1, point2, point3, sf::Color::Blue);

    Point point4({10, 10, 200});
    Point point5({20, 10, 200});
    Point point6({20, 20, 200});
    Triangle tri1(point4, point5, point6, sf::Color::Green);


    double leftovertime = 0;
    float dt = 0;
    const float PHYSICS_STEP = 1.0f/60;

    sf::Clock clock;


    
    while (window.isOpen()){
        sf::Event event;
        while (window.pollEvent(event)){
            switch (event.type)
            {
            case sf::Event::Closed:
                if(pipe){
                    pclose(pipe);
                }
                window.close();
                break;


            case sf::Event::KeyPressed:
                switch (event.key.code){
                case sf::Keyboard::W:
                    camera.moveForward = true;
                    break;
                case sf::Keyboard::S:
                    camera.moveBackward = true;
                    break;
                case sf::Keyboard::A:
                    camera.moveLeft = true;
                    break;
                case sf::Keyboard::D:
                    camera.moveRight = true;
                    break;
                case sf::Keyboard::Space:
                    camera.moveUp = true;
                    break;
                case sf::Keyboard::LShift:
                    camera.moveDown = true;
                    break;
                case sf::Keyboard::K:
                    pipe = popen(command.c_str(), "w");
                    break;
                }
                break;

                
            case sf::Event::KeyReleased:
                switch (event.key.code){
                    case sf::Keyboard::W:
                        camera.moveForward = false;
                        break;
                    case sf::Keyboard::S:
                        camera.moveBackward = false;
                        break;
                    case sf::Keyboard::A:
                        camera.moveLeft = false;
                        break;
                    case sf::Keyboard::D:
                        camera.moveRight = false;
                        break;
                    case sf::Keyboard::Space:
                        camera.moveUp = false;
                        break;
                    case sf::Keyboard::LShift:
                        camera.moveDown = false;
                        break;
                }
                break;
            }
        }

        dt = clock.restart().asSeconds();
        leftovertime += dt;
        while (leftovertime>PHYSICS_STEP){
            camera.integrate(PHYSICS_STEP);
            camera.vel *= 0.9f;            
            leftovertime -= PHYSICS_STEP;
        }

        sf::Vector3f forward = {cos(camera.verticalAngle)*sin(camera.horizontalAngle), sin(camera.verticalAngle), cos(camera.verticalAngle)*cos(camera.horizontalAngle)};
        sf::Vector3f right = -cap(crossProduct(forward, sf::Vector3f{0, 1, 0}));
        if ( camera.moveForward) {  camera.vel = forward * 100.0f; }
        else if ( camera.moveBackward ) { camera.vel = -forward* 80.0f; }

        if ( camera.moveLeft ) { camera.vel = -right* 60.0f; }
        else if ( camera.moveRight ) { camera.vel = right* 60.0f; }



        if ( camera.moveUp ) { camera.vel = sf::Vector3f{0, 1, 0} * 50.0f;}
        else if ( camera.moveDown ) { camera.vel = sf::Vector3f{0, -1, 0}* 80.0f;}

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2i center(screenWidth/2, screenHeight/2);

        float deltaX = mousePos.x - center.x;
        float deltaY = mousePos.y - center.y;

        sf::Mouse::setPosition(center, window);

        camera.horizontalAngle += deltaX* mouse.sensitivity;
        camera.verticalAngle -= deltaY*1.2* mouse.sensitivity;
        if (camera.verticalAngle>45*(3.14/180)){
            camera.verticalAngle = 45*(3.14/180);
        }
        if( camera.verticalAngle< -45*(3.14/180)){
            camera.verticalAngle = -45*(3.14/180);
        }

        window.clear();
for (int y = 0; y < screenHeight; y++){
    for (int x = 0; x < screenWidth; x++){
        int index = (x + y * screenWidth) * 4;

        windowPixels[index + 0] = 0;
        windowPixels[index + 1] = 0;
        windowPixels[index + 2] = 0;
        windowPixels[index + 3] = 255;
    }
}

        for (Cube& cube : cubes){
            cube.draw(windowPixels, camera, zbuffer);
        }

        for (Sphere& sphere:spheres){
            sphere.draw(window, camera);
        }
        // tri1.draw(windowPixels, camera, zbuffer);
        // tri.draw(windowPixels, camera, zbuffer);


        zbuffer.assign(screenHeight*screenWidth, 99999);

        windowRenderTexture.update(windowPixels.data());
        window.draw(windowSprite);
        
        

        if (pipe){
            texture.update(window);
            sf::Image image = texture.copyToImage();
            fwrite(image.getPixelsPtr(), 1, screenHeight*screenWidth*4, pipe);
        }
            window.display();
    }
    
    return 0;
}