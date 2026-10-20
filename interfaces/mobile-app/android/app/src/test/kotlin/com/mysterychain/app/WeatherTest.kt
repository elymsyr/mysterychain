package com.mysterychain.app

import org.junit.Assert.assertEquals
import org.junit.Test

class WeatherTest {
    @Test
    fun oneHour() {
        assertEquals("clear", Weather.KINDS[Weather.hour(2, 10.0, 20.0)]) // partly cloudy is still clear
        assertEquals("cloudy", Weather.KINDS[Weather.hour(45, 10.0, 20.0)]) // fog
        assertEquals("wind", Weather.KINDS[Weather.hour(3, 31.0, 20.0)]) // dry and windy
        assertEquals("wind", Weather.KINDS[Weather.hour(0, 10.0, 55.0)]) // gusty
        assertEquals("rain", Weather.KINDS[Weather.hour(61, 40.0, 70.0)]) // wet beats windy
        assertEquals("rain", Weather.KINDS[Weather.hour(56, 0.0, 0.0)]) // freezing drizzle
        assertEquals("snow", Weather.KINDS[Weather.hour(85, 0.0, 0.0)])
        assertEquals("storm", Weather.KINDS[Weather.hour(96, 0.0, 0.0)])
    }

    @Test
    fun worstHourWins() {
        assertEquals("rain", Weather.worst(listOf(0, 1, 80, 0), listOf(5.0, 5.0, 5.0, 5.0), listOf(9.0, 9.0, 9.0, 9.0)))
        assertEquals("wind", Weather.worst(listOf(0, 3, 0, 0), listOf(5.0, 5.0, 35.0, 5.0), listOf(9.0, 9.0, 9.0, 9.0)))
        assertEquals("cloudy", Weather.worst(listOf(3, 3, 3, 3), listOf(19.1), listOf()))  // short wind lists: calm
    }
}
