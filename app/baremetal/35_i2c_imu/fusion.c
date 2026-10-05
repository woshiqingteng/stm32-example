/**
 * @file    fusion.c
 * @brief   Attitude fusion: 6-axis Madgwick AHRS.
 *
 * The AHRS core is adapted from the MIT-licensed Fusion library by Seb Madgwick
 * (xio Technologies), https://github.com/xioTechnologies/Fusion, commit
 * a8d7224f36a0ec82345ef49a3db50e65f8d3bab8 (Fusion/FusionAhrs.c and
 * Fusion/FusionMath.h). Earth axes use the NWU convention and the Euler angles
 * are produced by FusionQuaternionToEuler(); the fixed sample rate of the
 * original is replaced by the dt passed to fusion_get_eulerian_angles().
 */

#include <math.h>

#include "fusion.h"
#include "imu.h"

/* x-io default settings. */
#define FUSION_GAIN             0.5f     /* FusionAhrsSettings.gain */
#define FUSION_INITIAL_GAIN     10.0f    /* INITIAL_STARTUP_GAIN */
#define FUSION_STARTUP_PERIOD   3.0f     /* STARTUP_PERIOD (s) */

/* Dynamic gyro-bias parameters. */
#define FUSION_ACC_1G_COUNT     4096.0f  /* counts per g at the configured +/-8g */
#define FUSION_DYN_ACC_MIN_G    0.9f     /* still detection: |acc| lower bound (g) */
#define FUSION_DYN_ACC_MAX_G    1.1f     /* still detection: |acc| upper bound (g) */
#define FUSION_DYN_GYRO_THR     200      /* still detection: |gyro| limit (~3 dps) */
#define FUSION_DYN_BIAS_ALPHA   0.01f    /* bias tracking gain when still */

static float g_q0 = 1.0f;                /* quaternion (w, x, y, z) */
static float g_q1 = 0.0f;
static float g_q2 = 0.0f;
static float g_q3 = 0.0f;
static float g_gain = FUSION_INITIAL_GAIN; /* ramps down to FUSION_GAIN */

static float g_gyro_bias[3];

/* Fast inverse square root (FusionMath.h: FusionFastInverseSqrt). */
static float fusion_inv_sqrt(float x)
{
    union {
        float   f;
        int32_t i;
    } u;

    u.f = x;
    u.i = 0x5F1F1412 - (u.i >> 1);

    return u.f * (1.69000231f - 0.714158168f * x * u.f * u.f);
}

/* Arc sine with clamping to avoid NaN (FusionMath.h: FusionArcSin). */
static float fusion_asin(float v)
{
    if (v <= -1.0f)
    {
        return -1.57079633f;
    }
    if (v >= 1.0f)
    {
        return 1.57079633f;
    }
    return asinf(v);
}

/* Normalise a quaternion in place. */
static void fusion_normalise(void)
{
    float n = g_q0 * g_q0 + g_q1 * g_q1 + g_q2 * g_q2 + g_q3 * g_q3;
    float inv = fusion_inv_sqrt(n);

    g_q0 *= inv;
    g_q1 *= inv;
    g_q2 *= inv;
    g_q3 *= inv;
}

void fusion_read_xyz(int16_t acc[3], int16_t gyro[3])
{
    int16_t raw_acc[3];
    int16_t raw_gyro[3];
    float   bias;

    (void)imu_read_raw(raw_acc, raw_gyro);

    acc[0] = raw_acc[0];
    acc[1] = raw_acc[1];
    acc[2] = raw_acc[2];

    bias    = g_gyro_bias[0];
    gyro[0] = (int16_t)((float)raw_gyro[0] - bias);
    bias    = g_gyro_bias[1];
    gyro[1] = (int16_t)((float)raw_gyro[1] - bias);
    bias    = g_gyro_bias[2];
    gyro[2] = (int16_t)((float)raw_gyro[2] - bias);
}

void fusion_update_dynamic_bias(const int16_t acc[3], const int16_t gyro[3])
{
    float   ax = (float)acc[0] / FUSION_ACC_1G_COUNT;
    float   ay = (float)acc[1] / FUSION_ACC_1G_COUNT;
    float   az = (float)acc[2] / FUSION_ACC_1G_COUNT;
    float   mag = sqrtf(ax * ax + ay * ay + az * az);
    uint8_t i;

    if ((mag < FUSION_DYN_ACC_MIN_G) || (mag > FUSION_DYN_ACC_MAX_G))
    {
        return;
    }

    for (i = 0U; i < 3U; i++)
    {
        if (((gyro[i] < 0) ? -gyro[i] : gyro[i]) >= FUSION_DYN_GYRO_THR)
        {
            return;
        }
    }

    for (i = 0U; i < 3U; i++)
    {
        g_gyro_bias[i] += FUSION_DYN_BIAS_ALPHA * (float)gyro[i];
    }
}

void fusion_get_eulerian_angles(const float acc[3], const float gyro[3], float *rpy, float dt)
{
    float rate;
    float norm;
    float hfx = 0.0f, hfy = 0.0f, hfz = 0.0f;
    float hx, hy, hz, sx, sy, sz, dq0, dq1, dq2, dq3;

    /* Startup gain ramp (FusionAhrs.c: Startup()). */
    rate = ((FUSION_INITIAL_GAIN - FUSION_GAIN) / FUSION_STARTUP_PERIOD) * dt;
    g_gain -= rate;
    if (g_gain < FUSION_GAIN)
    {
        g_gain = FUSION_GAIN;
    }

    /* Inclination feedback = residual(normalise(accelerometer), halfGravity).
     * halfGravity is the third column of the transposed rotation matrix scaled
     * by 0.5 (FusionAhrs.c: HalfGravity(), NWU). */
    norm = acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2];
    if (norm > 0.0f)
    {
        float hgx = g_q1 * g_q3 - g_q0 * g_q2;
        float hgy = g_q2 * g_q3 + g_q0 * g_q1;
        float hgz = g_q0 * g_q0 - 0.5f + g_q2 * g_q2;
        float inv = fusion_inv_sqrt(norm);
        float ax = acc[0] * inv;
        float ay = acc[1] * inv;
        float az = acc[2] * inv;
        float cx = ay * hgz - az * hgy;
        float cy = az * hgx - ax * hgz;
        float cz = ax * hgy - ay * hgx;

        if ((ax * hgx + ay * hgy + az * hgz) > 0.0f)
        {
            hfx = cx;                       /* error < 90 degrees */
            hfy = cy;
            hfz = cz;
        }
        else if ((cx * cx + cy * cy + cz * cz) > 0.0f)
        {
            float cinv = fusion_inv_sqrt(cx * cx + cy * cy + cz * cz);
            hfx = cx * cinv;
            hfy = cy * cinv;
            hfz = cz * cinv;
        }
    }

    /* halfAngularRate = halfGyro + halfFeedback * gain, then integrate:
     * q += q (x) (halfAngularRate * dt). */
    hx = gyro[0] * (0.5f * DEG2RAD) + hfx * g_gain;
    hy = gyro[1] * (0.5f * DEG2RAD) + hfy * g_gain;
    hz = gyro[2] * (0.5f * DEG2RAD) + hfz * g_gain;

    sx = hx * dt;
    sy = hy * dt;
    sz = hz * dt;

    dq0 = -g_q1 * sx - g_q2 * sy - g_q3 * sz;
    dq1 =  g_q0 * sx + g_q2 * sz - g_q3 * sy;
    dq2 =  g_q0 * sy - g_q1 * sz + g_q3 * sx;
    dq3 =  g_q0 * sz + g_q1 * sy - g_q2 * sx;

    g_q0 += dq0;
    g_q1 += dq1;
    g_q2 += dq2;
    g_q3 += dq3;

    fusion_normalise();

    /* ZYX Euler angles in degrees (FusionMath.h: FusionQuaternionToEuler()). */
    {
        float roll  = RAD2DEG * atan2f(g_q2 * g_q3 + g_q0 * g_q1,
                                       g_q0 * g_q0 + g_q3 * g_q3 - 0.5f);
        float pitch = RAD2DEG * fusion_asin(2.0f * (g_q0 * g_q2 - g_q1 * g_q3));
        float yaw   = RAD2DEG * atan2f(g_q1 * g_q2 + g_q0 * g_q3,
                                       g_q0 * g_q0 + g_q1 * g_q1 - 0.5f);

        rpy[0] = pitch;
        rpy[1] = roll;
        rpy[2] = yaw;
    }
}
