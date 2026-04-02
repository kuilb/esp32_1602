"""
MIDI转蜂鸣器旋律数组生成器
用于将MIDI文件转换为ESP32蜂鸣器可播放的旋律数组
"""

import mido
import math
from pathlib import Path

# MIDI音符号到音符名称的映射
MIDI_NOTE_NAMES = {
    51: "NOTE_DS3", 58: "NOTE_AS3", 63: "NOTE_DS4", 65: "NOTE_F4",
    66: "NOTE_FS4", 68: "NOTE_GS4", 70: "NOTE_AS4", 73: "NOTE_CS5",
    75: "NOTE_DS5"
}

# MIDI音符号到频率的转换 (A4 = 69 = 440Hz)
def midi_to_frequency(midi_note):
    """将MIDI音符号转换为频率(Hz)"""
    return round(440.0 * (2.0 ** ((midi_note - 69) / 12.0)))

def parse_midi_file(midi_path, max_notes=None, simplify=True):
    """
    解析MIDI文件并提取主旋律（单音化处理）
    使用时间轴扫描算法处理重叠音符和变速
    """
    mid = mido.MidiFile(midi_path)
    
    print(f"正在解析: {midi_path}")
    print(f"TPB (Ticks Per Beat): {mid.ticks_per_beat}")
    print(f"MIDI 总时长: {mid.length:.4f} 秒")

    # 1. 第一步：将所有音符事件转换为绝对时间（秒）
    # 过滤掉打击乐通道 (Channel 10, 索引为9)
    
    abs_notes = [] # 存储 (start_time, end_time, note_number)
    
    # 临时存储正在发声的音符 {note_number: start_time_in_seconds}
    # 由于可能存在多个轨道同一音高，使用列表存储
    active_notes = {} 
    
    current_time = 0.0
    current_tempo = 500000 # 默认 120 BPM
    
    # merge_tracks 会自动处理多轨道的时间顺序
    for msg in mido.merge_tracks(mid.tracks):
        # 计算当前消息的时间增量（秒）
        time_delta = mido.tick2second(msg.time, mid.ticks_per_beat, current_tempo)
        current_time += time_delta
        
        if msg.type == 'set_tempo':
            current_tempo = msg.tempo
            continue
            
        if msg.type == 'note_on' or msg.type == 'note_off':
            # 过滤打击乐通道 (Channel 9 is usually drums)
            if hasattr(msg, 'channel') and msg.channel == 9:
                continue
                
            note = msg.note
            velocity = msg.velocity if msg.type == 'note_on' else 0
            
            if msg.type == 'note_on' and velocity > 0:
                # 音符开始
                if note not in active_notes:
                    active_notes[note] = []
                active_notes[note].append(current_time)
            else:
                # 音符结束 (note_off 或 velocity=0)
                if note in active_notes and active_notes[note]:
                    start_time = active_notes[note].pop(0)
                    duration = current_time - start_time
                    # 忽略极短的杂音 (<10ms)
                    if duration > 0.01:
                        abs_notes.append({
                            'start': start_time,
                            'end': current_time,
                            'note': note
                        })

    # 2. 第二步：时间轴切片（单音化）
    # 我们需要把重叠的音符变成一条单音旋律线
    # 策略：在任何时间点，如果有多个音符在响，取音高最高的那个（通常是主旋律）
    
    # 收集所有关键时间点（音符开始或结束的时间）
    time_points = set()
    time_points.add(0.0) # 确保从 0 开始
    
    # 确保包含 MIDI 总时长，保证对齐
    # 有时候 mid.length 可能比最后一个音符结束时间短（虽然少见），取最大值
    max_end_time = mid.length
    for n in abs_notes:
        if n['end'] > max_end_time:
            max_end_time = n['end']
    time_points.add(max_end_time)

    for n in abs_notes:
        time_points.add(n['start'])
        time_points.add(n['end'])
    
    sorted_times = sorted(list(time_points))
    
    melody_segments = [] # (frequency, duration_ms)
    
    print(f"处理时间片段: {len(sorted_times)} 个关键帧")
    
    current_accumulated_ms = 0 # 记录当前生成的总毫秒数，用于消除浮点误差

    for i in range(len(sorted_times) - 1):
        t_start = sorted_times[i]
        t_end = sorted_times[i+1]
        
        # 计算目标结束时间点（毫秒）
        target_end_ms = int(t_end * 1000)
        
        # 当前片段的时长 = 目标结束时间 - 已生成的总时长
        # 这样可以自动补偿之前的取整误差，确保与 MIDI 时间点对齐
        duration_ms = target_end_ms - current_accumulated_ms
        
        if duration_ms <= 0: # 忽略极小或负的时间片
            continue
            
        current_accumulated_ms += duration_ms

        # 找出在这个时间段内正在播放的所有音符
        # 由于我们是根据音符边界切片的，在这个时间段内（不含边界）音符状态是恒定的
        # 使用中点来判断最稳健
        t_mid = (t_start + t_end) / 2
        playing_notes = [n['note'] for n in abs_notes if n['start'] <= t_mid and n['end'] >= t_mid]
        
        if playing_notes:
            # 取最高音作为主旋律
            best_note = max(playing_notes)
            freq = midi_to_frequency(best_note)
            melody_segments.append((freq, duration_ms))
        else:
            # 休止符
            melody_segments.append((0, duration_ms))

    # 3. 第三步：合并相邻的相同频率
    final_melody = []
    if melody_segments:
        current_freq, current_dur = melody_segments[0]
        
        for i in range(1, len(melody_segments)):
            next_freq, next_dur = melody_segments[i]
            
            if next_freq == current_freq:
                # 频率相同，合并时长
                current_dur += next_dur
            else:
                # 频率不同，保存当前，开始新的
                if current_dur > 0:
                    final_melody.append((current_freq, current_dur))
                current_freq = next_freq
                current_dur = next_dur
        
        # 添加最后一个
        if current_dur > 0:
            final_melody.append((current_freq, current_dur))

    # 限制音符数量
    if max_notes and len(final_melody) > max_notes:
        final_melody = final_melody[:max_notes]
        print(f"警告: 音符数量被截断为 {max_notes}")

    # 为了生成 detailed info，我们需要重建 notes 列表结构
    # 这里生成的 notes 只是为了日志显示，不再包含原始 MIDI 的复杂信息
    reconstructed_notes = []
    current_ms = 0
    for freq, dur in final_melody:
        if freq > 0:
            reconstructed_notes.append({
                'time': int(current_ms / 10), # 估算 tick，仅供参考
                'name': f"{freq}Hz",
                'frequency': freq,
                'duration': dur
            })
        current_ms += dur

    return final_melody, reconstructed_notes, mid.length

def generate_cpp_arrays(melody, notes, output_path, array_name="badAppleMelody"):
    """生成C++数组代码"""
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("// ==============================\n")
        f.write("// Bad Apple 蜂鸣器旋律数据\n")
        f.write("// 自动生成\n")
        f.write("// ==============================\n\n")

        f.write("#include <Arduino.h>\n\n")
        
        # 生成频率数组
        f.write(f"const uint16_t {array_name}Frequencies[] PROGMEM = {{\n    ")
        freq_strs = [str(freq) for freq, _ in melody]
        for i in range(0, len(freq_strs), 10):
            chunk = freq_strs[i:i+10]
            f.write(", ".join(chunk))
            if i + 10 < len(freq_strs):
                f.write(",\n    ")
        f.write("\n};\n\n")
        
        # 生成时长数组
        f.write(f"const uint16_t {array_name}Durations[] PROGMEM = {{\n    ")
        dur_strs = [str(dur) for _, dur in melody]
        for i in range(0, len(dur_strs), 10):
            chunk = dur_strs[i:i+10]
            f.write(", ".join(chunk))
            if i + 10 < len(dur_strs):
                f.write(",\n    ")
        f.write("\n};\n\n")
        
        # 生成长度常量
        f.write(f"const uint16_t {array_name}Length = {len(melody)};\n\n")
        
        # 添加使用示例
        f.write("/*\n")
        f.write(" * 使用示例:\n")
        f.write(f" * buzzerPlayMelody({array_name}Frequencies, {array_name}Durations, {array_name}Length, 50);\n")
        f.write(" */\n")
    
    print(f"生成C++代码: {output_path}")
    print(f"数组长度: {len(melody)} 个音符/休止符")

def generate_detailed_info(notes, output_path):
    """生成详细的音符信息文件"""
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("Bad Apple MIDI音符详细信息\n")
        f.write("=" * 60 + "\n\n")
        
        for i, note in enumerate(notes):
            f.write(f"[{i:4d}] 时间:{note['time']:6d}tick "
                   f"音符:{note['name']:12s} "
                   f"频率:{note['frequency']:4d}Hz "
                   f"时长:{note['duration']:4d}ms\n")
    
    print(f"生成详细信息: {output_path}")

if __name__ == "__main__":
    # 配置
    midi_file = "bad apple.mid"  # MIDI文件路径
    output_cpp = "bad_apple_melody.h"  # 输出的C++头文件
    output_info = "bad_apple_notes.txt"  # 音符详细信息
    
    # 参数设置
    MAX_NOTES = 32767  # 限制音符数量（太多会占用大量内存）
    SIMPLIFY = True   # 简化和弦为单音
    
    print("=" * 60)
    print("MIDI转蜂鸣器旋律数组生成器")
    print("=" * 60)
    
    # 检查文件
    if not Path(midi_file).exists():
        print(f"错误: 找不到MIDI文件 '{midi_file}'")
        exit(1)
    
    # 解析MIDI
    melody, notes, midi_length = parse_midi_file(midi_file, max_notes=MAX_NOTES, simplify=SIMPLIFY)
    
    # 生成C++代码
    generate_cpp_arrays(melody, notes, output_cpp)
    
    # 生成详细信息
    generate_detailed_info(notes, output_info)
    
    print("\n" + "=" * 60)
    print("完成!")
    print("=" * 60)
    
    # 统计信息
    total_duration = sum(dur for _, dur in melody)
    print(f"\n统计信息:")
    print(f"  总音符数: {len(melody)}")
    print(f"  生成时长: {total_duration/1000:.3f} 秒")
    print(f"  原始时长: {midi_length:.3f} 秒")
    print(f"  内存占用: {len(melody) * 4} 字节")
