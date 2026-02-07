#include "PlayerArrows.h"

void CPlayerArrows::DrawArrowTo(const Vec3& vFromPos, const Vec3& vToPos, Color_t tColor)
{
    const float flMaxDistance = Vars::Visuals::FOVArrows::MaxDist.Value;
    const float flMap = Math::RemapVal(vFromPos.DistTo(vToPos), flMaxDistance, flMaxDistance * 0.9f, 0.0f, 1.0f);
    tColor.a = static_cast<byte>(flMap * 255.0f);
    if (!tColor.a)
        return;

    Vec2 vCenter = { H::Draw.m_nScreenW * 0.5f, H::Draw.m_nScreenH * 0.5f };
    Vec3 vScreenPos;
    bool bOnScreen = SDK::W2S(vToPos, vScreenPos, true);

    if (bOnScreen)
    {
        float flMin = std::min(vCenter.x, vCenter.y);
        float flMax = std::max(vCenter.x, vCenter.y);
        float flDist = std::hypot(vScreenPos.x - vCenter.x, vScreenPos.y - vCenter.y);
        float flTransparency = 1.0f - std::clamp((flDist - flMin) / (flMax - flMin), 0.0f, 1.0f);
        tColor.a = static_cast<byte>(std::max(float(tColor.a) - flTransparency * 255.0f, 0.0f));
        if (!tColor.a)
            return;
    }

    if (Vars::Visuals::FOVArrows::OOBStyle.Value == Vars::Visuals::FOVArrows::OOBStyleEnum::Diamond)
    {
        Vec2 vDir = { vScreenPos.x - vCenter.x, vScreenPos.y - vCenter.y };
        float flAngle = std::atan2(vDir.y, vDir.x);
        float flOffset = Vars::Visuals::FOVArrows::Offset.Value;
        float flSize = H::Draw.Scale(30);
        float halfW = flSize * 0.35f;
        float tailLen = flSize * 0.4f;
        float tailTipW = flSize * 0.15f;

        Vec2 pts[6] = {
            { flOffset + flSize, 0.0f },
            { flOffset, halfW },
            { flOffset - tailLen, tailTipW },
            { flOffset - tailLen * 1.1f, 0.0f },
            { flOffset - tailLen, -tailTipW },
            { flOffset, -halfW }
        };

        Vec2 out[6];
        for (int i = 0; i < 6; ++i)
        {
            float x = pts[i].x * cosf(flAngle) - pts[i].y * sinf(flAngle);
            float y = pts[i].x * sinf(flAngle) + pts[i].y * cosf(flAngle);
            out[i] = { vCenter.x + x, vCenter.y + y };
        }

        H::Draw.FillPolygon({
            { { out[0].x, out[0].y } },
            { { out[1].x, out[1].y } },
            { { out[2].x, out[2].y } },
            { { out[3].x, out[3].y } },
            { { out[4].x, out[4].y } },
            { { out[5].x, out[5].y } }
            }, tColor);
        return;
    }
    else if (Vars::Visuals::FOVArrows::OOBStyle.Value == Vars::Visuals::FOVArrows::OOBStyleEnum::Arrow)
    {
        Vec3 vAngle;
        Math::VectorAngles({ vCenter.x - vScreenPos.x, vCenter.y - vScreenPos.y, 0 }, vAngle);
        const float flDeg = DEG2RAD(vAngle.y);
        const float flCos = std::cos(flDeg);
        const float flSin = std::sin(flDeg);

        float flOffset = -Vars::Visuals::FOVArrows::Offset.Value;
        float flScale = H::Draw.Scale(25);
        Vec2 v1 = { flOffset, flScale / 2 };
        Vec2 v2 = { flOffset, -flScale / 2 };
        Vec2 v3 = { flOffset - flScale * std::sqrt(3.0f) / 2, 0 };

        H::Draw.FillPolygon({
            { { vCenter.x + v1.x * flCos - v1.y * flSin, vCenter.y + v1.y * flCos + v1.x * flSin } },
            { { vCenter.x + v2.x * flCos - v2.y * flSin, vCenter.y + v2.y * flCos + v2.x * flSin } },
            { { vCenter.x + v3.x * flCos - v3.y * flSin, vCenter.y + v3.y * flCos + v3.x * flSin } }
            }, tColor);
    }
}

void CPlayerArrows::Run(CTFPlayer* pLocal)
{
    if (!Vars::Visuals::FOVArrows::Enabled.Value)
        return;

    Vec3 vLocalPos = pLocal->GetEyePosition();
    for (auto pEntity : H::Entities.GetGroup(EGroupType::PLAYERS_ENEMIES))
    {
        auto pPlayer = pEntity->As<CTFPlayer>();
        if ((pPlayer->IsDormant() && !H::Entities.GetDormancy(pPlayer->entindex())) ||
            !pPlayer->IsAlive() || pPlayer->IsAGhost() || pPlayer->InCond(TF_COND_STEALTHED))
            continue;

        Color_t tColor = H::Color.GetEntityDrawColor(pLocal, pEntity, Vars::Colors::Relative.Value);
        if (pPlayer->InCond(TF_COND_DISGUISED))
            tColor = Vars::Colors::Target.Value;

        DrawArrowTo(vLocalPos, pPlayer->GetCenter(), tColor);
    }
}
