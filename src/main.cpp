#include <SFML/Graphics.hpp>
#include <cmath>

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
            shape.setSize({ sqrt(distSquare(screenPoint1.x, screenPoint1.y, screenPoint2.x, screenPoint2.y)), 2});
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

class Cube:public PhysicsObject{
    public:
    std::vector<Point> points{8};
    float side;
    std::vector<sf::Vector3f> pointOffsets;
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
    }
    void draw(sf::RenderTarget& target, Camera& camera){        
        for (int i = 0; i < points.size(); i++){
            points[i].pos = pointOffsets[i]+pos;
        }
        std::vector<sf::Vector2f> screenPoints = resolvePoints(points, camera);
        for (int i = 0; i < lines.size(); i++){
            lines[i].draw(target, screenPoints[lines[i].point1], screenPoints[lines[i].point2]);
        }
    }
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

    std::string command = "ffmpeg -y -f rawvideo -pixel_format rgba -video_size " +std::to_string((int)screenWidth)+"x" + std::to_string((int)screenHeight) +" -framerate 60 -i - -c:v libx264 -preset ultrafast -pix_fmt yuv420p output.mp4";
    
    Camera camera;
    camera.pos = {0, 0, 0};
    camera.scale = 600;
    camera.vel = {0, 0, 0};
    camera.horizontalAngle = 0;

    Mouse mouse;

    std::vector<Cube> cubes;

    for (int i = 0; i < 20; i++){
        for (int j = 0; j < 20; j++){
            Cube cube({j*20, 0, i*20}, 20);
            cubes.push_back(cube);
        }
        
    }


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

        for (Cube& cube : cubes){
            cube.draw(window, camera);
        }
        if (pipe){
            texture.update(window);
            sf::Image image = texture.copyToImage();
            fwrite(image.getPixelsPtr(), 1, screenHeight*screenWidth*4, pipe);
        }
            window.display();
    }
    
    return 0;
}
