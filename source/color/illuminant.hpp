#pragma once

#include <string>
#include <string_view>

#include "cie.hpp"

namespace CIE
{
    namespace Illuminant
    {
        struct WhitePoint
        {
            const char* name;
            const glm::dvec2 xy;
        };

        inline constexpr WhitePoint white_points[]
        {
            { "A",        { 0.44757, 0.40745 } },
            { "B",        { 0.34842, 0.35161 } },
            { "C",        { 0.31006, 0.31616 } },
            { "D50",      { 0.34567, 0.35850 } },
            { "D55",      { 0.33242, 0.34743 } },
            { "D65",      { 0.31271, 0.32902 } },
            { "D75",      { 0.29902, 0.31485 } },
            { "E",        { 1.0/3.0, 1.0/3.0 } },
            { "F1",       { 0.31310, 0.33727 } },
            { "F2",       { 0.37208, 0.37529 } },
            { "F3",       { 0.40910, 0.39430 } },
            { "F4",       { 0.44018, 0.40329 } },
            { "F5",       { 0.31379, 0.34531 } },
            { "F6",       { 0.37790, 0.38835 } },
            { "F7",       { 0.31292, 0.32933 } },
            { "F8",       { 0.34588, 0.35875 } },
            { "F9",       { 0.37417, 0.37281 } },
            { "F10",      { 0.34609, 0.35986 } },
            { "F11",      { 0.38052, 0.37713 } },
            { "F12",      { 0.43695, 0.40441 } },
            { "LED-B1",   { 0.45600, 0.40780 } },
            { "LED-B2",   { 0.43570, 0.40120 } },
            { "LED-B3",   { 0.37560, 0.37230 } },
            { "LED-B4",   { 0.34220, 0.35020 } },
            { "LED-B5",   { 0.31180, 0.32360 } },
            { "LED-BH1",  { 0.44740, 0.40660 } },
            { "LED-RGB1", { 0.45570, 0.42110 } },
            { "LED-V1",   { 0.45600, 0.45480 } },
            { "LED-V2",   { 0.37810, 0.37750 } },
            { "MISSING",  { 0.32090, 0.15420 } }
        };

        enum
        {
            A, B, C,
            D50, D55, D65, D75,
            E,
            F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
            LED_B1, LED_B2, LED_B3, LED_B4, LED_B5, LED_BH1, LED_RGB1, LED_V1, LED_V2,
            NUM
        };
        
        inline constexpr glm::dvec3 whitePoint(size_t idx)
        {
            return XYZ(white_points[idx >= A && idx < NUM ? idx : NUM].xy, 1.0);
        }

        inline constexpr glm::dvec3 whitePoint(const char* name)
        {
            glm::dvec2 xy = white_points[NUM].xy;
            for (int i = 0; i < NUM; i++)
            {
                if (std::string_view(white_points[i].name) == name)
                {
                    xy = white_points[i].xy;
                    break;
                }
            }
            return XYZ(xy, 1.0);
        }
    }
}
