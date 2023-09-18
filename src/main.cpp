#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
//#include "serVivo.hpp"
#include <random>

using namespace std;

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;
const uint32_t HERBIVORE_START_ENERGY = 100;
const uint32_t CARNIVORE_START_ENERGY = 100;
const uint32_t ENERGY_FROM_PLANT = 30;
const uint32_t ENERGY_FROM_HERBIVORE = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

int novaIteracao = 0;

// Type definitions
enum entity_type_t
{
    emptyy,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
};

    static pos_t tentarMover(pos_t posicao, entity_type_t tipo);
    static bool tentarReproduzir(pos_t posicao, entity_type_t tipo);
    static bool tentarComer(pos_t posicao, entity_type_t tipo);
    static bool morrer(pos_t posicao, entity_type_t tipo);
    static void novaPlanta(pos_t posicao);
    static void novoHerbivoro(pos_t posicao);
    static void novoCarnivoro(pos_t posicao);
    static bool criarVida(entity_type_t tipo);

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {emptyy, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

bool random_action(float probability) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;
mutex mtx;


static pos_t tentarMover(pos_t posicao, entity_type_t tipo){
    vector<pos_t> posicoes_possiveis;
    pos_t direita = {posicao.i, posicao.j + 1};
    pos_t esquerda = {posicao.i, posicao.j - 1};
    pos_t cima = {posicao.i - 1, posicao.j};
    pos_t baixo = {posicao.i + 1, posicao.j};

    mtx.lock();

    if(direita.j < NUM_ROWS){ 
        if(entity_grid[direita.i][direita.j].type == emptyy)
            posicoes_possiveis.push_back(direita);
    }

    if(esquerda.j >= 0){
        if(entity_grid[esquerda.i][esquerda.j].type == emptyy)
            posicoes_possiveis.push_back(esquerda);
    }

    if(cima.i >= 0){
        if(entity_grid[cima.i][cima.j].type == emptyy)
            posicoes_possiveis.push_back(cima);
    }

    if(baixo.i < NUM_ROWS){
        if(entity_grid[baixo.i][baixo.j].type == emptyy)
            posicoes_possiveis.push_back(baixo);
    }

    if(posicoes_possiveis.size() > 0){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, posicoes_possiveis.size() - 1);
        int escolhida = dis(gen);
        pos_t posicao_escolhida = posicoes_possiveis[escolhida];

        if(tipo == herbivore){
            if(random_action(HERBIVORE_MOVE_PROBABILITY)){
                entity_grid[posicao_escolhida.i][posicao_escolhida.j] = entity_grid[posicao.i][posicao.j];
                entity_grid[posicao.i][posicao.j] = {emptyy, 0, 0};
                mtx.unlock();
                return posicao_escolhida;
            }
        }

        if(tipo == carnivore){
            if(random_action(CARNIVORE_MOVE_PROBABILITY)){
                entity_grid[posicao_escolhida.i][posicao_escolhida.j] = entity_grid[posicao.i][posicao.j];
                entity_grid[posicao.i][posicao.j] = {emptyy, 0, 0};
                mtx.unlock();
                return posicao_escolhida;
            }
        }
    }

    return posicao;
}

static bool tentarComer(pos_t posicao, entity_type_t tipo){
    vector<pos_t> posicoes_possiveis;
    pos_t direita = {posicao.i, posicao.j + 1};
    pos_t esquerda = {posicao.i, posicao.j - 1};
    pos_t cima = {posicao.i - 1, posicao.j};
    pos_t baixo = {posicao.i + 1, posicao.j};

    mtx.lock();

    if(direita.j < NUM_ROWS){ 
        if((tipo == herbivore && entity_grid[direita.i][direita.j].type == plant) ||
        (tipo == carnivore && entity_grid[direita.i][direita.j].type == herbivore))
            posicoes_possiveis.push_back(direita);
    }

    if(esquerda.j >= 0){
        if((tipo == herbivore && entity_grid[esquerda.i][esquerda.j].type == plant) ||
        (tipo == carnivore && entity_grid[esquerda.i][esquerda.j].type == herbivore))
            posicoes_possiveis.push_back(esquerda);
    }

    if(cima.i >= 0){
        if((tipo == herbivore && entity_grid[cima.i][cima.j].type == plant) ||
        (tipo == carnivore && entity_grid[cima.i][cima.j].type == herbivore))
            posicoes_possiveis.push_back(cima);
    }

    if(baixo.i < NUM_ROWS){
        if((tipo == herbivore && entity_grid[baixo.i][baixo.j].type == plant) ||
        (tipo == carnivore && entity_grid[baixo.i][baixo.j].type == herbivore))
            posicoes_possiveis.push_back(baixo);
    }

    if(posicoes_possiveis.size() > 0){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, posicoes_possiveis.size() - 1);
        int escolhida = dis(gen);
        pos_t posicao_escolhida = posicoes_possiveis[escolhida];

        if(tipo == herbivore ){
            if(random_action(HERBIVORE_EAT_PROBABILITY)){
                entity_grid[posicao_escolhida.i][posicao_escolhida.j] = {emptyy, 0, 0};
                entity_grid[posicao.i][posicao.j].energy += ENERGY_FROM_PLANT;
                mtx.unlock();
                return true;
            }
        }

        if(tipo == carnivore){
            if(random_action(CARNIVORE_EAT_PROBABILITY)){
                entity_grid[posicao_escolhida.i][posicao_escolhida.j] = {emptyy, 0, 0};
                entity_grid[posicao.i][posicao.j].energy += ENERGY_FROM_HERBIVORE;
                mtx.unlock();
                return true;
            }
        }
    }
    
    mtx.unlock();
    return false;
}

static bool tentarReproduzir(pos_t posicao, entity_type_t tipo)
{
    pos_t posicao_escolhida;

    vector<pos_t> posicoes_possiveis;
    pos_t direita = {posicao.i, posicao.j + 1};
    pos_t esquerda = {posicao.i, posicao.j - 1};
    pos_t cima = {posicao.i - 1, posicao.j};
    pos_t baixo = {posicao.i + 1, posicao.j};

    mtx.lock();
    if(direita.j < NUM_ROWS){ 
        if(entity_grid[direita.i][direita.j].type == emptyy)
            posicoes_possiveis.push_back(direita);
    }

    if(esquerda.j >= 0){
        if(entity_grid[esquerda.i][esquerda.j].type == emptyy)
            posicoes_possiveis.push_back(esquerda);
    }

    if(cima.i >= 0){
        if(entity_grid[cima.i][cima.j].type == emptyy)
            posicoes_possiveis.push_back(cima);
    }

    if(baixo.i < NUM_ROWS){
        if(entity_grid[baixo.i][baixo.j].type == emptyy)
            posicoes_possiveis.push_back(baixo);
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, posicoes_possiveis.size() - 1);
    int escolhida = dis(gen);
    posicao_escolhida = posicoes_possiveis[escolhida];

    if(tipo == plant){
        if(random_action(PLANT_REPRODUCTION_PROBABILITY)){
            entity_grid[posicao_escolhida.i][posicao_escolhida.j] = {plant, 0, 0};
            thread t1(novaPlanta, posicao_escolhida);
            t1.detach();
            mtx.unlock();
            return true;
        }
    }
    if(tipo == herbivore && entity_grid[posicao.i][posicao.j].energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION){
        if(random_action(HERBIVORE_REPRODUCTION_PROBABILITY)){
            entity_grid[posicao_escolhida.i][posicao_escolhida.j] = {herbivore, HERBIVORE_START_ENERGY, 0};
            thread t1(novoHerbivoro, posicao_escolhida);
            t1.detach();
            entity_grid[posicao.i][posicao.j].energy -= THRESHOLD_ENERGY_FOR_REPRODUCTION;
            mtx.unlock();
            return true;
        }
    }
    if(tipo == carnivore && entity_grid[posicao.i][posicao.j].energy >= THRESHOLD_ENERGY_FOR_REPRODUCTION){
        if(random_action(CARNIVORE_REPRODUCTION_PROBABILITY)){
            entity_grid[posicao_escolhida.i][posicao_escolhida.j] = {carnivore, CARNIVORE_START_ENERGY, 0};
            thread t1(novoCarnivoro, posicao_escolhida);
            t1.detach();
            entity_grid[posicao.i][posicao.j].energy -= THRESHOLD_ENERGY_FOR_REPRODUCTION;
            mtx.unlock();
            return true;
        }
    }
    
    mtx.unlock();
    return false;
}

static bool morrer(pos_t posicao){
    if(entity_grid[posicao.i][posicao.j].type == plant){
        if(entity_grid[posicao.i][posicao.j].age >= PLANT_MAXIMUM_AGE){
            entity_grid[posicao.i][posicao.j] = {emptyy, 0, 0};
            return true;
        }
    }

    if(entity_grid[posicao.i][posicao.j].type == herbivore){
        if(entity_grid[posicao.i][posicao.j].age >= HERBIVORE_MAXIMUM_AGE){
            entity_grid[posicao.i][posicao.j] = {emptyy, 0, 0};
            return true;
        }
        if(entity_grid[posicao.i][posicao.j].energy <= 0){
            entity_grid[posicao.i][posicao.j] = {emptyy, 0, 0};
            return true;
        }
    }

    if(entity_grid[posicao.i][posicao.j].type == carnivore){
        if(entity_grid[posicao.i][posicao.j].age >= CARNIVORE_MAXIMUM_AGE){
            entity_grid[posicao.i][posicao.j] = {emptyy, 0, 0};
            return true;
        }
        if(entity_grid[posicao.i][posicao.j].energy <= 0){
            entity_grid[posicao.i][posicao.j] = {emptyy, 0, 0};
            return true;
        }
    }

    return false;
}

static void novaPlanta(pos_t posicao){
    bool vivo = true;
    int iteracao = novaIteracao;
    while(vivo){
        if(novaIteracao > iteracao){
            iteracao++;
            vivo = entity_grid[posicao.i][posicao.j].type == plant 
                && !morrer(posicao);
            if(vivo){
                entity_grid[posicao.i][posicao.j].age++;
                tentarReproduzir(posicao, plant);
            } 
        }
    }
}

static void novoHerbivoro(pos_t posicao){
    bool vivo = true;
    int iteracao = novaIteracao;
    while(vivo){
        if(novaIteracao > iteracao){
            iteracao++;
            vivo = entity_grid[posicao.i][posicao.j].type == herbivore 
                && !morrer(posicao);
            if(vivo){
                entity_grid[posicao.i][posicao.j].age++;
                tentarComer(posicao, herbivore);
                tentarReproduzir(posicao, herbivore);
                pos_t novaPosicao = tentarMover(posicao, herbivore);
                if(novaPosicao.i != posicao.i || novaPosicao.j != posicao.j){
                    posicao = novaPosicao;
                }
            } 
        }
    }
}

static void novoCarnivoro(pos_t posicao){
    bool vivo = true;
    int iteracao = novaIteracao;
    while(vivo){
        if(novaIteracao > iteracao){
            iteracao++;
            vivo = entity_grid[posicao.i][posicao.j].type == carnivore 
                && !morrer(posicao);
            if(vivo){
                entity_grid[posicao.i][posicao.j].age++;
                tentarComer(posicao, carnivore);
                tentarReproduzir(posicao, carnivore);
                pos_t novaPosicao = tentarMover(posicao, carnivore);
                if(novaPosicao.i != posicao.i || novaPosicao.j != posicao.j){
                    posicao = novaPosicao;
                }
            } 
        }
    }
}

static bool criarVida(entity_type_t tipo){
    mtx.lock();

    if(entity_grid.size() == NUM_ROWS * NUM_ROWS){
        mtx.unlock();
        return false;
    }

    struct pos_t pos;
    bool alocado = false;
    std::random_device rd;
    std::mt19937 gen(rd());

    while(alocado == false){
        std::uniform_int_distribution<> dis(0, NUM_ROWS - 1);
        pos.i = dis(gen);
        pos.j = dis(gen);


        if(entity_grid[pos.i][pos.j].type == emptyy){
            if(tipo == plant){
                entity_grid[pos.i][pos.j] = {plant, 0, 0};
                thread t1(novaPlanta, pos);
                t1.detach();
            }
            if(tipo == herbivore){
                entity_grid[pos.i][pos.j] = {herbivore, HERBIVORE_START_ENERGY, 0};
                thread t1(novoHerbivoro, pos);
                t1.detach();
            }
            if(tipo == carnivore){
                entity_grid[pos.i][pos.j] = {carnivore, CARNIVORE_START_ENERGY, 0};
                thread t1(novoCarnivoro, pos);
                t1.detach();
            }

            alocado = true;
        }
    }
    mtx.unlock();
    return true;    
}

int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { emptyy, 0, 0}));
        
        // Create the entities
        // <YOUR CODE HERE>

        ////////////////////////////////////////////////////////////////////////////

        int plantas = (int)request_body["plants"];
        int herbivoros = (int)request_body["herbivores"];
        int carnivoros = (int)request_body["carnivores"];

        for(int i = 0; i < plantas; i++){
            if(!criarVida(plant)){
                break;
            }
        }
        for(int i = 0; i < herbivoros; i++){
            if(!criarVida(herbivore)){
                break;
            }
        }
        for(int i = 0; i < carnivoros; i++){
            if(!criarVida(carnivore)){
                break;
            }
        }

        ////////////////////////////////////////////////////////////////////////////

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behaviour of each entity
        
        // <YOUR CODE HERE>
        /////////////////////////////////////////////////////////////////////////

        novaIteracao ++;

        /////////////////////////////////////////////////////////////////////////
        
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}