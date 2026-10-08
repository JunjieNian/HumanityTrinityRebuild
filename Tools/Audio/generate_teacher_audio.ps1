param([string]$Python = 'D:\Anaconda\python.exe')
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$AudioOutput = Join-Path $ProjectRoot 'Assets\Audio\Teacher'
New-Item -ItemType Directory -Force -Path $AudioOutput | Out-Null
Add-Type -AssemblyName System.Speech
$Synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
try {
    $Voice = $Synth.GetInstalledVoices() | Where-Object { $_.VoiceInfo.Name -eq 'Microsoft Kangkang' } | Select-Object -First 1
    if (-not $Voice) {
        $Voice = $Synth.GetInstalledVoices() | Where-Object { $_.VoiceInfo.Culture.Name -eq 'zh-CN' } | Select-Object -First 1
    }
    if (-not $Voice) { throw 'A Chinese Windows speech voice is required to regenerate the teacher dialogue.' }
    $Synth.SelectVoice($Voice.VoiceInfo.Name)
    $Synth.Rate = -2
    $Synth.Volume = 90
    $Lines = [ordered]@{
        'SW_TeacherWarning' = '这么晚了，里面还有人吗？我检查一下。'
        'SW_TeacherClear' = '嗯，没人。门关好了。'
        'SW_TeacherCaught' = '谁在里面？出来！这么晚了还不回去。'
    }
    foreach ($Name in $Lines.Keys) {
        $Format = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(22050, [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)
        $Synth.SetOutputToWaveFile((Join-Path $AudioOutput ($Name + '_dry.wav')), $Format)
        $Synth.Speak($Lines[$Name])
        $Synth.SetOutputToNull()
    }
    $Manifest = [ordered]@{ voice = $Voice.VoiceInfo.Name; provenance = 'Offline Windows text-to-speech; fictional teacher, no real-person recording'; dialogue = $Lines }
    $Manifest | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 (Join-Path $AudioOutput 'dialogue.json')
} finally { $Synth.Dispose() }
& $Python (Join-Path $PSScriptRoot 'synthesize_teacher_effects.py')
if ($LASTEXITCODE -ne 0) { throw 'Teacher sound generation failed.' }
