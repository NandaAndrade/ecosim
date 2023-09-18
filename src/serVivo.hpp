#ifndef SERVIVO_HPP
#define SERVIVO_HPP

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

    entity_t(entity_type_t type, int32_t energy, int32_t age);
    ~entity_t();

    entity_type_t get_type();
    int32_t get_energy();
    int32_t get_age();

    void set_type(entity_type_t type);
    void set_energy(int32_t energy);
    void set_age(int32_t age);

    static pos_t tentarMover(pos_t posicao, entity_type_t tipo);
    static bool tentarReproduzir(pos_t posicao, entity_type_t tipo);
    static bool tentarComer(pos_t posicao, entity_type_t tipo);
    static bool morrer(pos_t posicao, entity_type_t tipo);
    static void novaPlanta(pos_t posicao);
    static void novoHerbivoro(pos_t posicao);
    static void novoCarnivoro(pos_t posicao);
    static bool criarVida(entity_type_t tipo);

};

#endif