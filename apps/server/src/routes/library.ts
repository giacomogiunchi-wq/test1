import { Router } from 'express';
import { promises as fs } from 'fs';
import path from 'path';
import { readUserLibrary } from '../storage';

const router = Router();

const blockPath = path.resolve(process.cwd(), '../web/public/block.json');

router.get('/', async (_req, res, next) => {
  try {
    const raw = await fs.readFile(blockPath, 'utf-8');
    const block = JSON.parse(raw);
    const userLibrary = await readUserLibrary();
    const merged = {
      machines: mergeById(block.machines ?? [], userLibrary.machines ?? []),
      sensors: mergeById(block.sensors ?? [], userLibrary.sensors ?? [])
    };
    res.json(merged);
  } catch (error) {
    next(error);
  }
});

function mergeById(base: any[], extra: any[]) {
  const map = new Map<string, any>();
  for (const item of base) {
    if (item?.id) {
      map.set(item.id, item);
    }
  }
  for (const item of extra) {
    if (item?.id) {
      map.set(item.id, { ...map.get(item.id), ...item });
    }
  }
  return Array.from(map.values());
}

export default router;
