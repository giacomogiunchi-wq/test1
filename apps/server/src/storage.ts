import { promises as fs } from 'fs';
import path from 'path';

const dataDir = path.resolve(process.cwd(), 'data');
const projectsDir = path.join(dataDir, 'projects');
const libraryPath = path.join(dataDir, 'library.json');

async function ensureDir(dir: string) {
  await fs.mkdir(dir, { recursive: true });
}

async function readJsonFile<T>(filePath: string, fallback: T): Promise<T> {
  try {
    const raw = await fs.readFile(filePath, 'utf-8');
    return JSON.parse(raw) as T;
  } catch (error: any) {
    if (error && error.code === 'ENOENT') {
      return fallback;
    }
    throw error;
  }
}

async function writeJsonFile(filePath: string, data: unknown) {
  await ensureDir(path.dirname(filePath));
  await fs.writeFile(filePath, JSON.stringify(data, null, 2), 'utf-8');
}

export type ProjectPayload = {
  id?: string;
  graph: unknown;
  families: string[];
  nSteps: number;
  libraryItems?: unknown;
};

export async function saveProject(payload: ProjectPayload) {
  const id = payload.id ?? createId();
  await ensureDir(projectsDir);
  await writeJsonFile(path.join(projectsDir, `${id}.json`), payload);
  return id;
}

export async function loadProject(id: string): Promise<ProjectPayload | null> {
  try {
    const raw = await fs.readFile(path.join(projectsDir, `${id}.json`), 'utf-8');
    return JSON.parse(raw) as ProjectPayload;
  } catch (error: any) {
    if (error && error.code === 'ENOENT') return null;
    throw error;
  }
}

export async function readUserLibrary(): Promise<{ machines: any[]; sensors: any[] }> {
  return readJsonFile(libraryPath, { machines: [], sensors: [] });
}

export async function mergeUserLibrary(library: { machines: any[]; sensors: any[] }) {
  await writeJsonFile(libraryPath, library);
}

function createId() {
  return `proj_${Date.now().toString(36)}_${Math.random().toString(36).slice(2, 8)}`;
}
