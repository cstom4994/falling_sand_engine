/**
 * A simple implementation of the marching squares algorithm that can identify
 * perimeters in an supplied byte array. The array of data over which this
 * instances of this class operate is not cloned by this class's constructor
 * (for obvious efficiency reasons) and should therefore not be modified while
 * the object is in use. It is expected that the data elements supplied to the
 * algorithm have already been thresholded. The algorithm only distinguishes
 * between zero and non-zero values.
 * 
 * @author Tom Gibara
 * Ported to C++ by Juha Reunanen
 *
 */

#pragma once

#include <box2d/b2_math.h>
#include <sstream>
#include <utility>
#include <vector>

namespace MarchingSquares {
    struct Direction
    {
        Direction() : x(0), y(0) {}
        Direction(int x, int y) : x(x), y(y) {}
        Direction(b2Vec2 vec) : x(vec.x), y(vec.y) {}
        int x;
        int y;
    };

    bool operator==(const Direction &a, const Direction &b) {
        return a.x == b.x && a.y == b.y;
    }

    Direction operator*(const Direction &direction, int multiplier) {
        return Direction(direction.x * multiplier, direction.y * multiplier);
    }

    Direction operator+(const Direction &a, const Direction &b) {
        return Direction(a.x + b.x, a.y + b.y);
    }

    Direction &operator+=(Direction &a, const Direction &b) {
        a.x += b.x;
        a.y += b.y;
        return a;
    }

    Direction MakeDirection(int x, int y) { return Direction(x, y); }

    Direction East() { return MakeDirection(1, 0); }
    Direction Northeast() { return MakeDirection(1, 1); }
    Direction North() { return MakeDirection(0, 1); }
    Direction Northwest() { return MakeDirection(-1, 1); }
    Direction West() { return MakeDirection(-1, 0); }
    Direction Southwest() { return MakeDirection(-1, -1); }
    Direction South() { return MakeDirection(0, -1); }
    Direction Southeast() { return MakeDirection(1, -1); }


    bool isSet(int x, int y, int width, int height, unsigned char *data) {
        return x <= 0 || x > width || y <= 0 || y > height
                       ? false
                       : data[(y - 1) * width + (x - 1)] != 0;
    }

    int value(int x, int y, int width, int height, unsigned char *data) {
        int sum = 0;
        if (isSet(x, y, width, height, data)) sum |= 1;
        if (isSet(x + 1, y, width, height, data)) sum |= 2;
        if (isSet(x, y + 1, width, height, data)) sum |= 4;
        if (isSet(x + 1, y + 1, width, height, data)) sum |= 8;
        return sum;
    }

    struct Result
    {
        int initialX = -1;
        int initialY = -1;
        std::vector<Direction> directions;
    };

    /**
     * Finds the perimeter between a set of zero and non-zero values which
     * begins at the specified data element. If no initial point is known,
     * consider using the convenience method supplied. The paths returned by
     * this method are always closed.
     *
     * The length of the supplied data array must exceed width * height,
     * with the data elements in row major order and the top-left-hand data
     * element at index zero.
     *
     * @param initialX
     *            the column of the data matrix at which to start tracing the
     *            perimeter
     * @param initialY
     *            the row of the data matrix at which to start tracing the
     *            perimeter
     * @param width
     *            the width of the data matrix
     * @param height
     *            the width of the data matrix
     * @param data
     *            the data elements
     *
     * @return a closed, anti-clockwise path that is a perimeter of between a
     *         set of zero and non-zero values in the data.
     * @throws std::runtime_error
     *             if there is no perimeter at the specified initial point.
     */

    Result FindPerimeter(int initialX, int initialY, int width, int height, unsigned char *data) {
        if (initialX < 0) initialX = 0;
        if (initialX > width) initialX = width;
        if (initialY < 0) initialY = 0;
        if (initialY > height) initialY = height;

        int initialValue = value(initialX, initialY, width, height, data);
        if (initialValue == 0 || initialValue == 15) {
            std::ostringstream error;
            error << "Supplied initial coordinates (" << initialX << ", " << initialY << ") do not lie on a perimeter.";
            //throw std::runtime_error(error.str());
            Result result;
            return result;
        }

        Result result;

        int x = initialX;
        int y = initialY;
        Direction previous = MakeDirection(0, 0);

        do {
            Direction direction;
            switch (value(x, y, width, height, data)) {
                case 1:
                    direction = North();
                    break;
                case 2:
                    direction = East();
                    break;
                case 3:
                    direction = East();
                    break;
                case 4:
                    direction = West();
                    break;
                case 5:
                    direction = North();
                    break;
                case 6:
                    direction = previous == North() ? West() : East();
                    break;
                case 7:
                    direction = East();
                    break;
                case 8:
                    direction = South();
                    break;
                case 9:
                    direction = previous == East() ? North() : South();
                    break;
                case 10:
                    direction = South();
                    break;
                case 11:
                    direction = South();
                    break;
                case 12:
                    direction = West();
                    break;
                case 13:
                    direction = North();
                    break;
                case 14:
                    direction = West();
                    break;
                default:
                    throw std::runtime_error("Illegal state");
            }
            if (direction == previous) {
                // compress
                result.directions.back() += direction;
            } else {
                result.directions.push_back(direction);
                previous = direction;
            }
            x += direction.x;
            y -= direction.y;// accommodate change of basis
        } while (x != initialX || y != initialY);

        result.initialX = initialX;
        result.initialY = initialY;

        return result;
    }

    /**
     * A convenience method that locates at least one perimeter in the data with
     * which this object was constructed. If there is no perimeter (ie. if all
     * elements of the supplied array are identically zero) then null is
     * returned.
     * 
     * @return a perimeter path obtained from the data, or null
     */

    Result FindPerimeter(int width, int height, unsigned char *data) {
        int size = width * height;
        for (int i = 0; i < size; i++) {
            if (data[i] != 0) {
                return FindPerimeter(i % width, i / width, width, height, data);
            }
        }
        Result result;
        return result;
    }

    Result FindPerimeter(int width, int height, unsigned char *data, int lookX, int lookY) {
        int size = width * height;
        for (int i = lookX + lookY * width; i < size; i++) {
            if (data[i] != 0) {
                //std::cout << (i%width) << " " << (i / width) << std::endl;
                return FindPerimeter(i % width, i / width, width, height, data);
            }
        }
        Result result;
        return result;
    }

    Direction FindEdge(int width, int height, unsigned char *data, int lookX, int lookY) {
        int size = width * height;
        for (int i = lookX + lookY * width; i < size; i++) {
            if (data[i] != 0) {
                //std::cout << (i%width) << " " << (i / width) << std::endl;
                int val = value(i % width, i / width, width, height, data);
                if (val != 0 && val != 15) {
                    return {i % width, i / width};
                }
            }
        }
        return {-1, -1};
    }

}// namespace MarchingSquares
