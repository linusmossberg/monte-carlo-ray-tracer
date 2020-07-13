#pragma once

#include <string>

#include <glm/vec2.hpp>

namespace CIE
{
    namespace Illuminant
    {
        enum IDX
        {
            A, B, C,
            D50, D55, D65, D75,
            E,
            F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
            LED_B1, LED_B2, LED_B3, LED_B4, LED_B5, LED_BH1, LED_RGB1, LED_V1, LED_V2,
            NUM
        };

        inline constexpr char* const strings[]
        {
            "A", "B", "C",
            "D50", "D55", "D65", "D75",
            "E",
            "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
            "LED-B1", "LED-B2", "LED-B3", "LED-B4", "LED-B5", "LED-BH1", "LED-RGB1", "LED-V1", "LED-V2"
        };

        inline IDX fromString(const std::string &I)
        {
            for (size_t i = IDX::A; i < IDX::NUM; i++)
            {
                if (strcmp(strings[i], I.c_str()) == 0) return (IDX)i;
            }
            return IDX::D65;
        }

        inline constexpr glm::dvec2 xy[]
        {
            { 0.44757, 0.40745 },
            { 0.34842, 0.35161 },
            { 0.31006, 0.31616 },
            { 0.34567, 0.35850 },
            { 0.33242, 0.34743 },
            { 0.31271, 0.32902 },
            { 0.29902, 0.31485 },
            { 1.0/3.0, 1.0/3.0 },
            { 0.31310, 0.33727 },
            { 0.37208, 0.37529 },
            { 0.40910, 0.39430 },
            { 0.44018, 0.40329 },
            { 0.31379, 0.34531 },
            { 0.37790, 0.38835 },
            { 0.31292, 0.32933 },
            { 0.34588, 0.35875 },
            { 0.37417, 0.37281 },
            { 0.34609, 0.35986 },
            { 0.38052, 0.37713 },
            { 0.43695, 0.40441 },
            { 0.45600, 0.40780 },
            { 0.43570, 0.40120 },
            { 0.37560, 0.37230 },
            { 0.34220, 0.35020 },
            { 0.31180, 0.32360 },
            { 0.44740, 0.40660 },
            { 0.45570, 0.42110 },
            { 0.45600, 0.45480 },
            { 0.37810, 0.37750 }
        };
    }
}