#include "StormSort.h"

bool comesBefore(const StormEvent& a, const StormEvent& b, SortField field) {
    switch (field) {
        case SortField::Damage:
            if (a.damageKnown != b.damageKnown) {
                return a.damageKnown;
            }
            if (a.damageKnown && a.propertyDamage != b.propertyDamage) {
                return a.propertyDamage > b.propertyDamage;
            }
            break;

        case SortField::Injuries:
            if (a.totalInjuries != b.totalInjuries) {
                return a.totalInjuries > b.totalInjuries;
            }
            break;

        case SortField::Date:
            if (a.dateKey != b.dateKey) {
                return a.dateKey > b.dateKey;
            }
            break;
    }

    return a.eventId < b.eventId;
}