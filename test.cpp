#include <iostream>
#include <string>
#include "src/types/object.h"
#include "src/types/integer.h"
#include "src/types/boolean.h"
using namespace std;


enum note {
    middleC,
    Csharp,
    Eflat,
};


class Instrument {
    public:
        virtual string what() const {
            return "Instrument";
        }
        virtual void play(note) const {
            cout << what() << "::play()" << endl;
        }
};


class Wind : public Instrument {
    public:
        string what() const {
            return "Wind";
        }
};


void tune(Instrument& i) {
    i.play(middleC);
}


int main() {
    Instrument instrum;
    Wind flute;

    tune(instrum);
    tune(flute);

    Instrument* ip = &flute;
    ip->play(middleC);

    cout << "\n\n";

    Object* registers[64];

    registers[0] = new Object();
    registers[1] = new Integer(2);
    registers[2] = new UnsignedInteger();
    registers[3] = new Boolean();

    cout << (*(registers[0])).str() << endl;
    cout << (*(registers[1])).str() << " " << (*static_cast<Integer*>(registers[1])).value() << endl;
    cout << (*(registers[2])).str() << " " << (*static_cast<UnsignedInteger*>(registers[2])).value() << endl;
    cout << (*(registers[3])).str() << " " << endl;

    return 0;
}
