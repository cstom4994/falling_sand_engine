#ifndef ME_PROGRESSOBJECT_H
#define ME_PROGRESSOBJECT_H

namespace MetaEngine::net {
class ProgressObject {
private:
    double currentAmt;
    double totalAmt;

public:
    ProgressObject(double currentAmt, double totalAmt) : currentAmt(currentAmt), totalAmt(totalAmt) {}

    double getCurrentAmt() const { return currentAmt; }

    double getTotalAmt() const { return totalAmt; }
};
}  // namespace MetaEngine::net

#endif  // ME_PROGRESSOBJECT_H
