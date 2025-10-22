// Fill out your copyright notice in the Description page of Project Settings.


#include "S1CastingBar.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "S1SkillComponent.h"

void US1CastingBar::StartCasting(const FSkillState& SkillState, uint64 CastEndTick)
{
    SetVisibility(ESlateVisibility::Visible);
    CastingBar_Fill->SetPercent(0.f);
    CastingBar_Text->SetText(FText::FromString(SkillState.name));

    InitializeCast(SkillState, CastEndTick);
}

void US1CastingBar::InitializeCast(const FSkillState& SkillState, uint64 CastEndTick)
{
    uint64 CurrentTick = static_cast<uint64>(FPlatformTime::Seconds() * 1000);

    CastDuration = SkillState.CastDuration; // 전체 CastDuration(ms)
    CastStartTick = SkillState.CastStartTick;            // 기본 시작 Tick

    // 서버 EndTick이 있는 경우
    if (CastEndTick > 0)
    {
        // 남은 시간 계산
        uint64 Remaining = CastEndTick > CurrentTick ? CastEndTick - CurrentTick : 0;

        // CastStartTick을 서버 기준으로 재조정
        CastStartTick = CastEndTick - CastDuration;
    }

    ElapsedCastTime = CurrentTick - CastStartTick; // 초기 진행률
    bIsCasting = true;
}

void US1CastingBar::CancelCasting()
{
    SetVisibility(ESlateVisibility::Hidden);
    ElapsedCastTime = 0;
    CastDuration = 0;
    CastStartTick = 0;
    bIsCasting = false;
}

void US1CastingBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bIsCasting || CastDuration == 0)
        return;

    UpdateCastBar(InDeltaTime);
}

void US1CastingBar::UpdateCastBar(float DeltaTime)
{
    uint64 CurrentTick = static_cast<uint64>(FPlatformTime::Seconds() * 1000);

    // Tick 기준으로 실제 경과 시간 계산
    ElapsedCastTime = CurrentTick - CastStartTick;

    // 목표 진행률 계산 (절대값 기준)
    float Percent = FMath::Clamp(static_cast<float>(ElapsedCastTime) / CastDuration, 0.f, 1.f);

    // ProgressBar 바로 적용
    if (CastingBar_Fill)
    {
        CastingBar_Fill->SetPercent(Percent);
    }

    // 서버 EndTick 기준으로 종료
    if (CurrentTick >= CastStartTick + CastDuration)
    {
        SetVisibility(ESlateVisibility::Hidden);
        bIsCasting = false;
        CastDuration = 0;
        ElapsedCastTime = 0;
        CastStartTick = 0;
    }
}