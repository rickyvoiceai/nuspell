#!/usr/bin/env python3
import sys
import argparse
import json
import time
import os
import threading
from openai import OpenAI

SYSTEM_PROMPT = """You are a word validator for an English dictionary. Given a single English word, determine if it is a valid English word that should be added to a spelling dictionary.

Consider valid: proper nouns (names, places, brands), technical terms, loanwords commonly used in English, and any legitimate English word.

Consider invalid: typos, gibberish, random letter combinations, or words that are clearly misspellings of other words.

Respond ONLY with a JSON object (no markdown, no explanation):
{"valid": true/false, "score": 1-5, "category": "proper_noun|common_word|technical|loanword|abbreviation|invalid", "note": "brief explanation"}

Score: 5=very common/important, 4=common, 3=somewhat common, 2=rare, 1=very rare/obscure"""

def make_client(args):
    return OpenAI(base_url=args.server, api_key=args.apikey, timeout=60)

def validate_word(word, logprob, args):
    client = make_client(args)
    for attempt in range(args.max_retries):
        try:
            response = client.chat.completions.create(
                model=args.model,
                messages=[
                    {"role": "system", "content": SYSTEM_PROMPT},
                    {"role": "user", "content": f"Word: '{word}'"}
                ],
                max_tokens=128,
                temperature=0.0,
                stream=False
            )
            content = response.choices[0].message.content.strip()
            content = content.replace("```json", "").replace("```", "").strip()
            try:
                data = json.loads(content)
            except json.JSONDecodeError:
                if attempt < args.max_retries - 1:
                    time.sleep(args.retry_delay * (attempt + 1))
                    continue
                data = {"valid": False, "score": 0, "category": "json_error", "note": f"raw: {content[:120]}"}
            data["word"] = word
            data["logprob"] = logprob
            return data
        except Exception as e:
            if attempt == args.max_retries - 1:
                return {"word": word, "logprob": logprob, "valid": False, "score": 0, "category": "error", "note": str(e)}
            time.sleep(args.retry_delay * (attempt + 1))
    return {"word": word, "logprob": logprob, "valid": False, "score": 0, "category": "error", "note": "max retries exceeded"}

def load_existing_results(path):
    done = set()
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    obj = json.loads(line)
                    done.add(obj["word"])
                except (json.JSONDecodeError, KeyError):
                    pass
    return done

def main():
    parser = argparse.ArgumentParser(description="Validate words via LLM for dictionary addition")
    parser.add_argument("--input", required=True, help="Input file: '<logprob> <word>' per line")
    parser.add_argument("--output", required=True, help="Output .jsonl file")
    parser.add_argument("--server", default="http://198.18.18.100:8080/v1", help="OpenAI-compatible API base URL")
    parser.add_argument("--apikey", default="voiceai", help="API key")
    parser.add_argument("--model", default="qwen3.5", help="Model name")
    parser.add_argument("--max-retries", type=int, default=3, help="Max retries per word")
    parser.add_argument("--retry-delay", type=float, default=2.0, help="Base seconds between retries (exponential backoff)")
    parser.add_argument("--batch-delay", type=float, default=0.1, help="Tiny throttle between batches (seconds)")
    parser.add_argument("--workers", type=int, default=4, help="Concurrent workers")
    parser.add_argument("--start", type=int, default=0, help="Start from this line number (0-based)")
    parser.add_argument("--count", type=int, default=0, help="Process this many words (0=all)")
    args = parser.parse_args()

    words = []
    with open(args.input, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) >= 2:
                logprob = float(parts[0])
                word = parts[1]
                words.append((logprob, word))

    if args.start > 0:
        words = words[args.start:]
    if args.count > 0:
        words = words[:args.count]

    tmp_path = args.output + ".tmp"
    done = load_existing_results(tmp_path)
    todo = [(lp, w) for lp, w in words if w not in done]

    print(f"Total: {len(words)}, already done: {len(done)}, remaining: {len(todo)}", flush=True)

    batch_size = args.workers
    total_batches = (len(todo) + batch_size - 1) // batch_size
    processed = 0

    with open(tmp_path, "a", encoding="utf-8") as out_f:
        lock = threading.Lock()

        def worker(logprob, word):
            result = validate_word(word, logprob, args)
            with lock:
                out_f.write(json.dumps(result, ensure_ascii=False) + "\n")
                out_f.flush()

        for batch_idx in range(total_batches):
            start = batch_idx * batch_size
            end = min(start + batch_size, len(todo))
            batch = todo[start:end]

            threads = []
            for logprob, word in batch:
                t = threading.Thread(target=worker, args=(logprob, word))
                threads.append(t)
                t.start()

            for t in threads:
                t.join()

            processed += len(batch)
            if batch_idx % 10 == 0 or batch_idx == total_batches - 1:
                print(f"  Batch {batch_idx+1}/{total_batches}: {processed}/{len(todo)} done", flush=True)

            if batch_idx < total_batches - 1 and args.batch_delay > 0:
                time.sleep(args.batch_delay)

    os.replace(tmp_path, args.output)

    valid_count = 0
    invalid_count = 0
    error_count = 0
    with open(args.output, "r", encoding="utf-8") as f:
        for line in f:
            obj = json.loads(line.strip())
            if obj.get("category") in ("error", "json_error"):
                error_count += 1
            elif obj.get("valid", False):
                valid_count += 1
            else:
                invalid_count += 1

    print(f"\nDone: {valid_count} valid, {invalid_count} invalid, {error_count} errors ({valid_count + invalid_count + error_count} total)")
    print(f"Output written to {args.output}")

if __name__ == "__main__":
    main()