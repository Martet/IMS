#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>

#define FIELD_WIDTH 1000
#define FIELD_HEIGHT 1000
#define BASE_X 500
#define BASE_Y 100
#define CAPTURE_TIME 50

int teamCount[] = {20, 20};
int captureTimer = 0;

sf::RenderWindow window(sf::VideoMode(FIELD_WIDTH, FIELD_HEIGHT), "Battle simulator");

bool rngBool(float probability) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution d(probability);
    return d(gen);
}

float normalFloat(float mean, float stddev) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> d(mean, stddev);
    return d(gen);
}

float expDist(float lambda, float x) {
    return std::exp(-lambda * x);
}

class Agent {
    public:
        int x, y, team, id;
        bool inBase;

        Agent(int x, int y, int team, int id) {
            this->x = x;
            this->y = y;
            this->team = team;
            this->id = id;
            this->inBase = false;

            this->shape = sf::CircleShape(5);
            this->shape.setFillColor(team ? sf::Color::Red : sf::Color::Blue);
        }

        void update(std::vector<Agent> &agents, std::vector<Agent *> &baseAgents) {
            if (this->inBase) {
                // Enemy capturing base
                if(team == 1) {
                    captureTimer++;
                } else {
                    // Leave base if no other enemies
                    if(inBaseCount(1, baseAgents) == 0) {
                        removeFromBase(this->id, &baseAgents);
                        this->inBase = false;
                    }
                }

                // Eliminate enemies in base
                if(inBaseCount(!this->team, baseAgents) && rngBool(0.05)) {
                    eliminate(!this->team, true, agents, &baseAgents);
                    if(inBaseCount(1, baseAgents) == 0) {
                        captureTimer = 0;
                    }
                }
            } else {
                // Get in base
                if((this->team == 1 && getDistanceBase() < 10) ||
                   (this->team == 0 && inBaseCount(1, baseAgents) && getDistanceBase() < 10)) {
                    this->inBase = true;
                    baseAgents.push_back(this);
                    return;
                }

                // Enemies are in base, decide if move towards it
                int inBase = inBaseCount(1, baseAgents);
                if(this->team == 0 && rngBool(1 - expDist(0.5, inBase))) {
                    move(normalFloat(getDirectionBase(), 0.1 * M_PI), normalFloat(30, 2));
                    return;
                }

                // Spot enemies
                auto near = spotAgents(agents, 0.01, !this->team);
                if(near.size() > 0) {
                    shoot(nearestAgent(near, !this->team), agents, 0.005);
                } else { // Move towards base
                    if(this->team == 1) {
                        move(normalFloat(getDirectionBase(), 0.3 * M_PI), normalFloat(15, 2));
                    } else {
                        float distanceMean = std::pow(getDistanceBase() - 50, 3) / 1000;
                        if(distanceMean < -40) distanceMean = -40;
                        else if(distanceMean > 40) distanceMean = 40;
                        move(normalFloat(getDirectionBase(), 0.3 * M_PI), normalFloat(distanceMean, 2));
                    }
                }
            }
        }

        void draw() {
            if (!this->inBase) {
                this->shape.setPosition(this->x - 5, this->y - 5);
                window.draw(this->shape);
            }
        }

        friend std::ostream &operator<<(std::ostream &Str, Agent const &a) { 
            return Str << "ID=" << a.id << " x=" << a.x << " y=" << a.y << " team=" << a.team << " inBase=" << a.inBase;
        }

    private:
        sf::CircleShape shape;

        void move(float direction, float distance) {
            this->x += distance * std::cos(direction);
            this->y += distance * std::sin(direction);
            if(this->x < 0) this->x = 0;
            if(this->y < 0) this->y = 0;
            if(this->x > FIELD_WIDTH) this->x = FIELD_WIDTH;
            if(this->y > FIELD_HEIGHT) this->y = FIELD_HEIGHT;
        }

        void shoot(Agent *a, std::vector<Agent> &agents, float probability) {
            sf::Vertex line[2];
            line[0].position = sf::Vector2f(this->x, this->y);
            line[0].color  = sf::Color::Black;
            line[1].position = sf::Vector2f(a->x, a->y);
            line[1].color = sf::Color::Black;
            window.draw(line, 2, sf::Lines);
            if(a->team != this->team && rngBool(expDist(probability, this->getDistance(*a)))){
                eliminate(a->id, false, agents, nullptr);
            }
        }

        Agent *nearestAgent(std::vector<Agent *> &agents, int team) {
            Agent *nearest = this;
            float minDist = 1000000;
            for(Agent *a : agents) {
                float dist = this->getDistance(*a);
                if(dist < minDist && a != this && a->team == team && !a->inBase) {
                    minDist = dist;
                    nearest = a;
                }
            }
            return nearest;
        }

        std::vector<Agent *> spotAgents(std::vector<Agent> &agents, float probability, int team){
            std::vector<Agent *> seenAgents;
            for(auto &a : agents){
                if(&a != this && a.team == team && rngBool(expDist(probability, this->getDistance(a)))){
                    seenAgents.push_back(&a);
                }
            }
            return seenAgents;
        }

        float getDistance(Agent &a) {
            return std::sqrt(std::pow(this->x - a.x, 2) + std::pow(this->y - a.y, 2));
        }

        float getDirection(Agent &a) {
            return std::atan2(a.y - this->y, a.x - this->x);
        }

        float getDistanceBase() {
            return std::sqrt(std::pow(BASE_X - this->x, 2) + std::pow(BASE_Y - this->y, 2));
        }

        float getDirectionBase() {
            return std::atan2(BASE_Y - this->y, BASE_X - this->x);
        }

        int inBaseCount(int team, std::vector<Agent *> agents) {
            int count = 0;
            for(auto &a : agents) {
                if(a->team == team && a->inBase) {
                    count++;
                }
            }
            return count;
        }

        void removeFromBase(int id, std::vector<Agent *> *agents) {
            for(auto it = agents->begin(); it != agents->end(); ++it) {
                if((*it)->id == id) {
                    agents->erase(it);
                    return;
                }
            }
        }

        void eliminate(int id, bool inBase, std::vector<Agent> &agents, std::vector<Agent *> *baseAgents) {
            for(auto it = agents.begin(); it != agents.end(); ++it){
                if(it->id == id){
                    agents.erase(it);
                    teamCount[it->team]--;
                    break;
                }
            }
            if(inBase) {
                removeFromBase(id, baseAgents);
            }
        }
};

int main() {
    window.setVerticalSyncEnabled(false);
    std::vector<Agent> agents;
    std::vector<Agent *> baseAgents;
    sf::CircleShape baseShape(20);
    baseShape.setPosition(BASE_X - 20, BASE_Y - 20);
    baseShape.setFillColor(sf::Color::Black);
    for(int i = 0; i < 20; i++) {
        agents.push_back(Agent(40 * i + 100, 100, 0, i));
        agents.push_back(Agent(40 * i + 100, 900, 1, i + 20));
    }
    int time = 0;
    sf::Clock clock;
    while(window.isOpen() && teamCount[0] > 0 && teamCount[1] > 0 && time < 8640 && captureTimer < 100) {
        sf::Event event;
        while(window.pollEvent(event)) {
            if(event.type == sf::Event::Closed)
                window.close();
        }

        if(clock.getElapsedTime().asSeconds() > 0.3) {
            clock.restart();

            window.clear(sf::Color::White);
            for (auto &agent : agents) {
                agent.update(agents, baseAgents);
            }
            for (auto &agent : agents) {
                agent.draw();
            }

            std::cout << time * 10 << " " << teamCount[0] << " " << teamCount[1] << std::endl;
            
            window.draw(baseShape);
            window.display();
            time++;
        }
    }
    if(teamCount[0] <= 0) {
        std::cerr << "Defenders won";
    } else if(teamCount[1] <= 0) {
        std::cerr << "Attackers won (defenders eliminated)";
    } else if(time >= 8640) {
        std::cerr << "Time limit reached";
    } else if(captureTimer >= 100) {
        std::cerr << "Attackers won (base captured)";
    }
    std::cerr << std::endl;
    return 0;
}
