#include <math.h>
#include <raylib.h>
#include "ecs.hpp"

struct Position
{
    Vector2 position;
};

struct Sprite
{
    Texture texture;
    Vector2 frameSize;
    int frameCount;

    Sprite(Texture texture, Vector2 frameSize, int frameCount = 1)
	: texture(texture), frameSize(frameSize), frameCount(frameCount)
    {}
};

struct Dynamic
{
    Vector2 velocity;
    Vector2 acceleration;
    float dampening = 1.0f;
};

struct Health
{
    int health;
    int maxHealth;
    int damage;
};

struct Dragndrop
{
    bool dragged;
};

struct Radius
{
    float radius;
};

struct Keyboard
{
    bool active;
};

struct Bullet
{
    int damage;
    Entity shooter;
};

struct Lifetime
{
    int timeToLive;
};

struct Homing
{
    float distance;
    Vector2 position;
};


void DebugDrawCallback(int i, int amount, const char* name)
{
    DrawText(TextFormat("\"%s\": x%d", name, amount), 1, i * 12, 10, BLACK);
}

float GetRandomAngle() {
    int deg = rand() % 360;
    float rad = (float)deg * DEG2RAD;

    return rad;
}

float GetDistance(const Vector2 &p1, const Vector2 &p2) {
    return sqrt((p2.x - p1.x) * (p2.x - p1.x) + 
	    (p2.y - p1.y) * (p2.y - p1.y));
}


void SystemRender(EntityManager& manager)
{
    auto func = [](Entity e, auto &p) -> void {
	DrawCircleLinesV(p.position, 2, BLACK);
    };
    manager.ForEach<Position>(func);
}

void SystemRenderSprites(EntityManager& manager)
{
    auto func = [](Entity e, Sprite &s, Position &p) -> void {
	DrawTexture(s.texture, p.position.x - s.frameSize.x / 2.0f, p.position.y - s.frameSize.y / 2.0f, WHITE);
    };
    manager.ForEach<Sprite, Position>(func);
}

void SystemRenderRaduis(EntityManager& manager)
{
    auto func = [](Entity e, Radius &r, Position &p) -> void {
	DrawCircleLinesV(p.position, r.radius, BLACK);
    };
    manager.ForEach<Radius, Position>(func);
}

void SystemLifetime(EntityManager& manager)
{
    auto func = [&](Entity e, Lifetime &l) -> void {
	l.timeToLive--;
	if (l.timeToLive <= 0)
	    manager.KillEntity(e);
    };
    manager.ForEach<Lifetime>(func);
}


void SystemBullet(EntityManager& manager)
{
    manager.ForEach<Bullet, Position>([&](Entity e1, Bullet &b, Position &p1) {
		manager.ForEach<Health, Radius, Position>([&](Entity e2, Health &h2, Radius &r2, Position &p2) {

		    if (e1 == e2) return;
		    if (e2 == b.shooter ) return;
		    float distance = GetDistance(p1.position, p2.position);
		    if (distance < r2.radius)
		    {
			h2.damage += b.damage;
			manager.KillEntity(e1);
		    }
		});
	    });
}

void SystemCollistion(EntityManager& manager)
{
    manager.ForEach<Radius, Position>([&](Entity e1, Radius &r1, Position &p1)
	{
	    manager.ForEach<Radius, Position>([&](Entity e2, Radius &r2, Position &p2) {
		if (e1 == e2) return;
		float distance = GetDistance(p1.position, p2.position);
		if (distance < r1.radius + r2.radius ) {
		    float overlap = 0.5 * (distance - r2.radius - r1.radius);
		    p2.position.x += overlap * (p1.position.x - p2.position.x) / distance;
		    p2.position.y += overlap * (p1.position.y - p2.position.y) / distance;

		    p1.position.x -= overlap * (p1.position.x - p2.position.x) / distance;
		    p1.position.y -= overlap * (p1.position.y - p2.position.y) / distance;
		}
	    });
	});

}

void SystemMovement(EntityManager& manager)
{
    auto func = [](Entity e, Dynamic &d, Position &p) -> void {
	if (p.position.x < 0 || p.position.x > GetScreenWidth())
	    d.velocity.x = -d.velocity.x;
	if (p.position.y < 0 || p.position.y > GetScreenHeight())
	    d.velocity.y = -d.velocity.y;
	p.position.x += d.velocity.x;
	p.position.y += d.velocity.y;

	d.velocity.x *= d.dampening;
	d.velocity.y *= d.dampening;
    };
    manager.ForEach<Dynamic, Position>(func);
}

void SystemHoming(EntityManager &manager)
{
    manager.ForEach<Homing, Dynamic, Position>([](Entity e, Homing &h, Dynamic &d, Position &p) -> void {

		float distance = GetDistance(p.position, h.position);
		if (distance > h.distance)
		    return;
		Vector2 diff = {h.position.x - p.position.x, h.position.y - p.position.y};
		Vector2 n = {diff.x / distance, diff.y / distance};

		float mag = sqrt(d.velocity.x*d.velocity.x + d.velocity.y * d.velocity.y);
		d.velocity = {n.x * mag, n.y * mag};

	    });

}

void SystemKeyboard(EntityManager& manager)
{
    
    manager.ForEach<Keyboard, Dynamic>([](Entity e, Keyboard &k, Dynamic &d) {
		d.velocity.x = 0;
		d.velocity.y = 0;
		if (IsKeyDown(KEY_W)) {
		    d.velocity.y -= 2.0f;
		}
		if (IsKeyDown(KEY_S)) {
		    d.velocity.y += 2.0f;
		}
		if (IsKeyDown(KEY_A)) {
		    d.velocity.x -= 2.0f;
		}
		if (IsKeyDown(KEY_D)) {
		    d.velocity.x += 2.0f;
		}
	    });
}

void SystemHealth(EntityManager& manager)
{
    auto func = [&](Entity e, Health &h) -> void {
	h.health -= h.damage;
	h.damage = 0;

	if (h.health <= 0) {
	    h.health = 0;
	    manager.KillEntity(e);
	}
	
    };
    manager.ForEach<Health>(func);
}

void SystemDragndrop(EntityManager& manager)
{
    Vector2 mouse = GetMousePosition();

    manager.ForEach<Dragndrop, Radius, Dynamic, Position>(
	    [&](Entity e, Dragndrop &d, Radius &r, Dynamic &v, Position &p)
	    {
		if ( 
			(mouse.x - p.position.x) * (mouse.x - p.position.x) + 
			(mouse.y - p.position.y) * (mouse.y - p.position.y) < r.radius * r.radius) {
		    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			d.dragged = true;
		}

		if (d.dragged) {
		    v.velocity = {0, 0};
		    p.position.x += GetMouseDelta().x;
		    p.position.y += GetMouseDelta().y;
		    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
			d.dragged = false;
		}
	    }
	);
    
}

void SystemHBars(EntityManager& manager)
{
    auto func = [](Entity e, Health &h, Position &p) -> void {
	float r = 10;
	DrawRectangle(p.position.x - 10, p.position.y - r - 6, 20, 4, RED);
	DrawRectangle(p.position.x - 10, p.position.y - r - 6, 20 * h.health / h.maxHealth, 4, GREEN);
    };
    manager.ForEach<Health, Position>(func);
}

void SystemDelete(EntityManager& manager)
{
    if (!IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	return;

    Vector2 mouse = GetMousePosition();
    manager.ForEach<Radius, Position>([&](Entity e, Radius &r, Position &p) -> void {
		if ((mouse.x - p.position.x) * (mouse.x - p.position.x) + 
			(mouse.y - p.position.y) * (mouse.y - p.position.y) < r.radius * r.radius) {
		    manager.KillEntity(e);
		}
	    });
}

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Entity Component System");
    SetTargetFPS(60);

    Texture texBomb = LoadTexture("assets/st2-bomb1.png");
    Texture texTux = LoadTexture("assets/st2-tux.png");
    Texture texSpark = LoadTexture("assets/spark.png");

    EntityManager manager;

    manager.RegisterComponentType<Position>();
    manager.RegisterComponentType<Dynamic>();
    manager.RegisterComponentType<Health>();
    manager.RegisterComponentType<Radius>();
    manager.RegisterComponentType<Dragndrop>();
    manager.RegisterComponentType<Sprite>();
    manager.RegisterComponentType<Keyboard>();
    manager.RegisterComponentType<Bullet>();
    manager.RegisterComponentType<Lifetime>();
    manager.RegisterComponentType<Homing>();

    Entity player;

    // Add a bunch or random entities
    for (int i = 0; i < 20; i++)
    {
	Entity e = manager.AddEntity();
	manager.AddComponent(e, Position({ float(rand() % screenWidth), float(rand() % screenHeight) }));
	if (i > 3) {
	    manager.AddComponent(e, Radius({10 + float(rand() % 10)}));
	}

	float speed = (rand() % 100 + 30) / 60.0f;

	if ( i >= 7 ) {
	    manager.AddComponent<Dynamic>(e, Dynamic{ 
		    { cos(GetRandomAngle()) * speed, sin(GetRandomAngle()) * speed }, {0, 0}, (i > 14) ? 1.0f : 0.99f} );
	}

	if ( i >= 10 ) {
	    manager.AddComponent<Health>(e, {rand() % 100, 100, 0});
	    // componentsRadius[e].radius += rand() % 20;
	    manager.AddComponent<Dragndrop>(e, {false});
	}

	if (i >= 15 && i != 19) {
	    manager.AddComponent<Sprite>(e, Sprite(texBomb, {(float)texBomb.width, (float)texBomb.height}));
	}

	if (i == 19) {
	    manager.AddComponent<Keyboard>(e, Keyboard{1});
	    manager.AddComponent<Sprite>(e, Sprite(texTux, {(float)texTux.width, (float)texTux.height}));
	    player = e;
	}
   }

    while (!WindowShouldClose())
    {
	// Shoot a bullet when SPACE is pressed
	if (IsKeyPressed(KEY_SPACE))
	{
	    auto pos = manager.GetComponent<Position>(player);
	    if (pos) {
		Vector2 p = pos->position;
		Vector2 m = GetMousePosition();

		float d = GetDistance(p, m);
		Vector2 diff = {m.x - p.x, m.y - p.y};
		Vector2 n = {diff.x / d, diff.y / d};

		float speed = 3;

		Entity bullet = manager.AddEntity();
		// manager.AddComponent<Position>(bullet, Position{p.x + n.x * 25, p.y + n.y * 25});
		manager.AddComponent<Position>(bullet, Position{p.x, p.y});
		manager.AddComponent<Dynamic>(bullet, Dynamic{{n.x * speed, n.y * speed}, {0,0}, 1.0f});
		manager.AddComponent<Lifetime>(bullet, Lifetime{3 * 60});
		manager.AddComponent<Bullet>(bullet, {20, player});
		manager.AddComponent<Homing>(bullet, {150, {screenWidth / 2.0f, screenHeight / 2.0f}});
		manager.AddComponent<Sprite>(bullet, Sprite(texSpark, {(float)texSpark.width, (float)texSpark.height}));
	    }
	    
	}

	SystemKeyboard(manager);

	SystemHoming(manager);
	SystemMovement(manager);
	
	SystemBullet(manager);
	SystemHealth(manager);
	SystemLifetime(manager);
	SystemCollistion(manager);
	SystemDragndrop(manager);

	SystemDelete(manager);

	BeginDrawing();

	ClearBackground(RAYWHITE);

	SystemRender(manager);
	SystemRenderRaduis(manager);
	SystemRenderSprites(manager);
	SystemHBars(manager);

	manager.DrawDebugInfo(DebugDrawCallback);

	EndDrawing();

	manager.CleanupDead();
    }

    CloseWindow();
    return 0;
}
