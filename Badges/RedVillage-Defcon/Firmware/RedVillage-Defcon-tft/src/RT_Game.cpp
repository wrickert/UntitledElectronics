#include "RT_Game.h"
#include <string>
#include <vector>

//Vector table for the RoomLayout. 1 is a door, 0 is wall. Order is NESW
std::vector<std::int8_t> RoomLayout{
    //0
    0b0010,
    //1
    0b1110,
    //2
    0b0101,
    //3
    0b0001,
    //4
    0b1111,
    //5
    0b0101,
    //6
    0b0100,
    //7
    0b0011,
    //8
    0b0010,
    //9
    0b1011,
    //10
    0b0101,
    //11
    0b0110,
    //12
    0b1100,
    //13
    0b1011,
    //14
    0b1000,
    //15
    0b1000,
    //16
    0b0011,
    //17
    0b1110,
    //18
    0b1100,
    //19
    0b1011,
    //20
    0b1000
};

//Array to keep track of which rooms need information logos
//                  1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1
int bossRoom[21] = {0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,1};

//Multidimensional lookup table for where each room leads.
//-1 means wall. Order is NESW
int RoomMap[21][4] = {
    //0
    {-1,-1,1,-1},
    //1
    {0,2,4,-1},
    //2
    {-1,3,-1,1},
    //3
    {-1,-1,-1,2},
    //4
    {1,7,9,5},
    //5
    {-1,4,-1,6},
    //6
    {-1,5,-1,-1},
    //7
    {-1,-1,12,4},
    //8
    {-1,-1,13,-1},
    //9
    {4,-1,15,10},
    //10
    {-1,9,-1,11},
    //11
    {-1,10,17,-1},
    //12
    {7,13,-1,-1},
    //13
    {8,-1,14,12},
    //14
    {13,-1,-1,-1},
    //15
    {9,-1,-1,-1},
    //16
    {-1,-1,19,16},
    //17
    {11,16,18,-1},
    //18
    {17,19,-1,-1},
    //19
    {16,-1,20,18},
    //20
    {19,-1,-1,-1}
};

int get_IsBossRoom(int RoomNumber){
    return bossRoom[RoomNumber];
}

int get_RoomLayout(int RoomNumber){
    return RoomLayout[RoomNumber];
}

int get_Room_North(int RoomNumber){
    return RoomMap[RoomNumber][0];
}

int get_Room_East(int RoomNumber){
    return RoomMap[RoomNumber][1];
}

int get_Room_South(int RoomNumber){
    return RoomMap[RoomNumber][2];
}

int get_Room_West(int RoomNumber){
    return RoomMap[RoomNumber][3];
}

bool is_on_info(int x, int y){
    if(x > 115 && y > 67 && x < 205 && y < 135)
        return true;
    else
        return false;
}
